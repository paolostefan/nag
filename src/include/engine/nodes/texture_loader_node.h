#ifndef NAG_ENGINE_NODES_TEXTURE_LOADER_NODE_H
#define NAG_ENGINE_NODES_TEXTURE_LOADER_NODE_H

#include <string>

#include "GL/glew.h"
#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "IconsFontAwesome6.h"
#include "stb_image.h"

#include "engine/nodes/visual_node.h"

/**
 * @class TextureLoaderNode
 * @brief Loads a PNG or JPEG from disk and exposes it as a Texture* stream.
 *
 * The image is decoded via stb_image and uploaded to an OpenGL texture once,
 * then re-used every frame. Reloading is triggered only when the file path
 * changes (dirty flag pattern), so evaluate() is cheap at steady state.
 *
 * The node does NOT own a RenderTarget — it uploads directly to a raw GL
 * texture. VisualNode is therefore not a suitable base; this node inherits
 * from Node directly and manages its own GL resource.
 *
 * File dialog:
 *   draw_properties() calls OpenDialog() on the singleton.
 *   NodePropertiesPanel::render() must call display_file_dialog() every frame
 *   so the dialog is shown even when the properties panel is the focused window.
 *
 * Output: Texture*
 */
struct TextureLoaderNode : VisualNode {
  std::string file_path;

  TextureLoaderNode() {
    type = NodeType::TextureLoader;
    name = "Texture Loader";

    VisualNode::initialize(1,1); // dummy 1x1 texture to start with; will be replaced on load
  }

  // ── Node interface ────────────────────────────────────────────────────────

  [[nodiscard]] std::string_view type_name() const noexcept override { return "Texture Loader"; }

  void evaluate() override {
    if (path_dirty_) {
      reload_texture();
      path_dirty_ = false;
    }

    if (!outputs.empty()) {
      outputs[0].set_texture(output_texture.is_valid() ? &output_texture : nullptr);
    }
  }

  // ── VisualNode overrides ──────────────────────────────────────────────────

  void render() override {
    // No rendering needed; this node just loads a texture and outputs it.
    spdlog::info("TextureLoaderNode::render()");
  }

  // ── File dialog helpers ───────────────────────────────────────────────────

  /**
   * @brief Opens the ImGuiFileDialog for this node.
   * Call from draw_properties().
   */
  void open_file_dialog() const {
    IGFD::FileDialogConfig cfg;
    cfg.path = file_path.empty() ? "." : file_path.substr(0, file_path.find_last_of("/\\"));
    cfg.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(dialog_key(),
                                            "Load Texture",
                                            kImageFilter,
                                            cfg);
  }

  /**
   * @brief Processes the file dialog result.
   * Must be called every frame from NodePropertiesPanel::render()
   * (or any other site that runs unconditionally each frame).
   *
   * @return true if a new path was selected this frame.
   */
  bool display_file_dialog() {
    constexpr ImVec2 kDialogSize{600.f, 400.f};

    if (!ImGuiFileDialog::Instance()->Display(dialog_key(),
                                              ImGuiWindowFlags_NoCollapse,
                                              kDialogSize)) {
      return false; // dialog not open or not yet confirmed
    }

    bool accepted = false;
    if (ImGuiFileDialog::Instance()->IsOk()) {
      set_path(ImGuiFileDialog::Instance()->GetFilePathName());
      accepted = true;
    }

    ImGuiFileDialog::Instance()->Close();
    return accepted;
  }

  /**
   * @brief Sets a new file path and marks the texture as dirty.
   */
  void set_path(const std::string &path) {
    if (path == file_path) return;
    file_path = path;
    path_dirty_ = true;
  }

  // ── Serialization ─────────────────────────────────────────────────────────

  [[nodiscard]] nlohmann::json serialize_params() const override {
    nlohmann::json j;
    j["file_path"] = file_path;
    return j;
  }

  [[nodiscard]] OperationResult deserialize_params(const nlohmann::json &j) override {
    try {
      if (j.contains("file_path")) {
        set_path(j["file_path"].get<std::string>());
      }
      return OperationResult::ok();
    } catch (const std::exception &e) {
      return OperationResult::error(
        std::string("Failed to deserialize TextureLoaderNode params: ") + e.what());
    }
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<TextureLoaderNode> create(const std::string &path = {}) {
    auto node = std::make_unique<TextureLoaderNode>();

    node->add_output(DataType::Texture, "texture");

    if (!path.empty()) {
      node->set_path(path);
    }

    return node;
  }

private:
  static constexpr auto *kImageFilter = "Image files{.png,.jpg,.jpeg,.bmp,.tga}";

  // GLuint gl_texture_{0};
  bool path_dirty_{false};
  std::string last_error_;

  // Unique dialog key per node instance — avoids conflicts when multiple
  // TextureLoaderNodes exist in the same graph.
  [[nodiscard]] std::string dialog_key() const {
    return "texture_loader_dialog_" + std::to_string(id);
  }

  /**
   * @brief Loads the image from disk and uploads it to OpenGL.
   * If a texture already exists, it is deleted first.
   * On failure, last_error_ is set with a descriptive message.
   */
  void reload_texture() {
    if (file_path.empty()) return;

    render_target->free_texture();
    last_error_.clear();

    // stb_image: flip vertically to match OpenGL UV convention (origin = bottom-left)
    stbi_set_flip_vertically_on_load(true);

    int width{0}, height{0}, channels{0};
    unsigned char *data = stbi_load(file_path.c_str(),
                                    &width,
                                    &height,
                                    &channels,
                                    4);
    if (!data) {
      last_error_ = std::string("stb_image: ") + stbi_failure_reason();
      spdlog::error("TextureLoaderNode: failed to load '{}': {}",
                    file_path, last_error_);
      return;
    }

    render_target->resize(width, height);

    // Upload to OpenGL
    glBindTexture(GL_TEXTURE_2D, render_target->get_texture());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    output_texture.texture_id = render_target->get_texture();
    output_texture.width = width;
    output_texture.height = height;

    spdlog::info("TextureLoaderNode: loaded '{}' ({}x{}, {} ch)", file_path, width, height, channels);
  }
};

#endif  // NAG_ENGINE_NODES_TEXTURE_LOADER_NODE_H

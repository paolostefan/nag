#ifndef NAG_ENGINE_NODES_TEXTURE_LOADER_NODE_H
#define NAG_ENGINE_NODES_TEXTURE_LOADER_NODE_H

#include <string>

#include "GL/glew.h"
#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include "IconsFontAwesome6.h"
#include "stb_image.h"

#include "engine/nodes/node.h"
#include "engine/render_target.h"

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
struct TextureLoaderNode : Node {
  std::string file_path;

  TextureLoaderNode() {
    type = NodeType::TextureLoader;
    name = "Texture Loader";
  }

  ~TextureLoaderNode() override {
    free_texture();
  }

  // ── Node interface ────────────────────────────────────────────────────────

  void evaluate() override {
    if (path_dirty_) {
      reload_texture();
      path_dirty_ = false;
    }

    // Always push current texture pointer into the output stream.
    if (!outputs.empty()) {
      if (auto *s = dynamic_cast<Stream<Texture *> *>(outputs[0].stream.get())) {
        s->update(output_texture_.is_valid() ? &output_texture_ : nullptr);
      }
    }

    mark_inputs_consumed();
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
    ImGuiFileDialog::Instance()->OpenDialog(
      dialog_key(), "Load Texture", kImageFilter, cfg);
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

    if (!ImGuiFileDialog::Instance()->Display(
      dialog_key(), ImGuiWindowFlags_NoCollapse, kDialogSize)) {
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

  // ── Properties panel ──────────────────────────────────────────────────────

  void draw_properties(NodeGraph & /*graph*/, CommandHistory & /*history*/) override {
    // Current path (read-only display)
    if (file_path.empty()) {
      ImGui::TextDisabled("No file loaded.");
    } else {
      // Show only the filename, tooltip shows full path
      const std::string filename = file_path.substr(file_path.find_last_of("/\\") + 1);
      ImGui::TextUnformatted(filename.c_str());
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", file_path.c_str());
      }
    }

    // Texture info
    if (output_texture_.is_valid()) {
      ImGui::TextDisabled("%d x %d", output_texture_.width, output_texture_.height);
    }

    // Error message if last load failed
    if (!last_error_.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
      ImGui::TextWrapped("%s", last_error_.c_str());
      ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Browse...")) {
      open_file_dialog();
    }

    // Reload button — useful if the file on disk has changed
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ROTATE_RIGHT "  Reload")) {
      path_dirty_ = true;
    }
  }

  // ── Factory ───────────────────────────────────────────────────────────────

  static std::unique_ptr<TextureLoaderNode> create(const std::string &path = {}) {
    auto node = std::make_unique<TextureLoaderNode>();

    node->add_typed_output<Texture *>("texture");

    if (!path.empty()) {
      node->set_path(path);
    }

    return node;
  }

private:
  static constexpr const char *kImageFilter = "Image files{.png,.jpg,.jpeg,.bmp,.tga}";

  Texture output_texture_;
  GLuint gl_texture_{0};
  bool path_dirty_{false};
  std::string last_error_;

  // Unique dialog key per node instance — avoids conflicts when multiple
  // TextureLoaderNodes exist in the same graph.
  [[nodiscard]] std::string dialog_key() const {
    return "texture_loader_dialog_" + std::to_string(id);
  }

  void reload_texture() {
    free_texture();
    last_error_.clear();

    if (file_path.empty()) return;

    // stb_image: flip vertically to match OpenGL UV convention (origin = bottom-left)
    stbi_set_flip_vertically_on_load(true);

    int width{0}, height{0}, channels{0};
    unsigned char *data = stbi_load(file_path.c_str(), &width, &height, &channels, 4);

    if (!data) {
      last_error_ = std::string("stb_image: ") + stbi_failure_reason();
      spdlog::error("TextureLoaderNode: failed to load '{}': {}", file_path, last_error_);
      return;
    }

    // Upload to OpenGL
    glGenTextures(1, &gl_texture_);
    glBindTexture(GL_TEXTURE_2D, gl_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    output_texture_ = Texture{gl_texture_, width, height};
    spdlog::info("TextureLoaderNode: loaded '{}' ({}x{}, {} ch)", file_path, width, height, channels);
  }

  void free_texture() {
    if (gl_texture_ != 0) {
      glDeleteTextures(1, &gl_texture_);
      gl_texture_ = 0;
    }
    output_texture_ = Texture{};
  }
};

#endif  // NAG_ENGINE_NODES_TEXTURE_LOADER_NODE_H

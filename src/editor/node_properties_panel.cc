#include "editor/node_properties_panel.h"

#include "imgui.h"
#include "imnodes.h"

#include "editor/node_properties_renderers.h"
#include "engine/node_graph.h"
#include "engine/nodes/texture_loader_node.h"

void NodePropertiesPanel::render(NodeGraph &graph, CommandHistory &history) {
  const int selected_count = ImNodes::NumSelectedNodes();

  int selected_id = -1;
  if (selected_count == 1) {
    ImNodes::GetSelectedNodes(&selected_id);
  }

  last_selected_id_ = selected_id;

  ImGui::PushID("NodePropertiesPanel");

  if (selected_count == 0) {
    ImGui::TextDisabled(ICON_FA_BAN " No node selected");
  } else if (selected_count > 1) {
    ImGui::TextDisabled(ICON_FA_PEOPLE_GROUP " Multiple nodes selected");
  } else {
    if (Node *node = graph.find_node(selected_id); !node) {
      ImGui::TextDisabled("(node not found)");
    } else {
      ImGui::TextUnformatted(node->name.c_str());
      ImGui::SameLine();
      ImGui::TextDisabled("(id %d)", node->id);
      ImGui::Separator();

      draw_node_properties(*node, graph, history);

      if (auto *tl = dynamic_cast<TextureLoaderNode *>(node)) {
        tl->display_file_dialog();
      }
    }
  }

  ImGui::PopID();
}

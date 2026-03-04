#include "editor/node_properties_panel.h"

#include "imgui.h"
#include "imnodes.h"

#include "engine/node_graph.h"

void NodePropertiesPanel::render(NodeGraph &graph, CommandHistory &history) {
  // -------------------------------------------------------------------------
  // Read current ImNodes selection (runs every frame — real-time update).
  // -------------------------------------------------------------------------
  const int selected_count = ImNodes::NumSelectedNodes();

  int selected_id = -1;
  if (selected_count == 1) {
    // GetSelectedNodes expects a pre-allocated buffer.
    ImNodes::GetSelectedNodes(&selected_id);
  }

  // Track selection changes if needed downstream.
  last_selected_id_ = selected_id;

  // -------------------------------------------------------------------------
  // Render the collapsible panel.
  // -------------------------------------------------------------------------
  if (!ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    return; // Panel is collapsed — nothing more to draw.
  }

  ImGui::PushID("NodePropertiesPanel");

  if (selected_count == 0) {
    ImGui::TextDisabled("No node selected.");
  } else if (selected_count > 1) {
    ImGui::TextDisabled("Multiple nodes selected.");
  } else {
    // Exactly one node selected.
    Node *node = graph.find_node(selected_id);
    if (!node) {
      // Should not happen in a consistent graph, but guard defensively.
      ImGui::TextDisabled("(node not found)");
    } else {
      // Show node name and type as a small header inside the panel.
      ImGui::TextUnformatted(node->name.c_str());
      ImGui::SameLine();
      ImGui::TextDisabled("(id %d)", node->id);
      ImGui::Separator();

      // Delegate all parameter widgets to the node itself.
      // draw_properties() is a no-op by default; concrete nodes override it.
      node->draw_properties(graph, history);
    }
  }

  ImGui::PopID();
}

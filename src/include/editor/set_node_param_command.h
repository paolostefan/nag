#ifndef NAG_EDITOR_SET_NODE_PARAM_COMMAND_H
#define NAG_EDITOR_SET_NODE_PARAM_COMMAND_H

#include <functional>
#include <string>
#include <utility>

#include "editor/command.h"
#include "engine/node_graph.h"

/// @file set_node_param_command.h
/// @brief Generic undo/redo command for a single node parameter change.
///
/// Usage (inside draw_properties or a PropertyWidget helper):
/// @code
///   history.execute(graph,
///     std::make_unique<SetNodeParamCommand<float>>(
///       node_id_,
///       "Blur Radius",
///       value_before,
///       node.radius,
///       [](Node& n, float v) {
///         static_cast<BlurNode&>(n).radius = v;
///       }));
/// @endcode

template<typename T>
class SetNodeParamCommand final : public ICommand {
public:
  using Setter = std::function<void(Node &, T)>;

  /// @param node_id   ID of the node owning the parameter.
  /// @param param_name  Human-readable name shown in undo history.
  /// @param before    Value captured at the start of the drag/edit.
  /// @param after     Value captured at the end of the drag/edit.
  /// @param setter    Callable that writes the value back onto the node.
  SetNodeParamCommand(const int node_id, std::string param_name, T before, T after,
                      Setter setter)
    : node_id_(node_id),
      param_name_(std::move(param_name)),
      before_(std::move(before)),
      after_(std::move(after)),
      setter_(std::move(setter)) {
  }

  // ICommand interface ---------------------------------------------------------

  bool execute(NodeGraph &graph) override {
    return apply(graph, after_);
  }

  bool undo(NodeGraph &graph) override {
    return apply(graph, before_);
  }

  [[nodiscard]] std::string description() const override {
    return "Set " + param_name_;
  }

private:
  bool apply(NodeGraph &graph, const T &value) {
    Node *node = graph.find_node(node_id_);
    if (!node) return false;
    setter_(*node, value);
    // Re-evaluate the node so its output stream version advances and the
    // change propagates to downstream nodes on the next incremental pass.
    node->evaluate();
    return true;
  }

  int node_id_;
  std::string param_name_;
  T before_;
  T after_;
  Setter setter_;
};

#endif // NAG_EDITOR_SET_NODE_PARAM_COMMAND_H

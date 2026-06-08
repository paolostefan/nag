#ifndef NAG_ENGINE_SERIALIZATION_JSON_GRAPH_SERIALIZER_H
#define NAG_ENGINE_SERIALIZATION_JSON_GRAPH_SERIALIZER_H

#include "engine/serialization/graph_serializer.h"

/**
 * @brief JSON implementation of graph serialization.
 *
 * File format:
 * {
 *   "version": "1.0",
 *   "metadata": { ... },
 *   "nodes": [ ... ],
 *   "links": [ ... ]
 * }
 */
class JsonGraphSerializer : public IGraphSerializer {
public:
  static constexpr auto kFormatVersion = "1.0";

  [[nodiscard]] OperationResult save(const NodeGraph &graph,
                                     const std::string &path) const override;

  /**
   * @brief Load a graph from a JSON file.
   *
   * @param graph Graph to load into (will be cleared before loading)
   * @param path Path to load from
   * @return OperationResult indicating success or failure, with error message on failure
   */
  [[nodiscard]] OperationResult load(NodeGraph &graph,
                                     const std::string &path) const override;

  /**
   * @brief Serialize a single node to JSON.
   *
   * @param node Node to serialize
   * @return JSON object
   */
  [[nodiscard]] static nlohmann::json serialize_node(const Node *node);

  /**
   * @brief Deserialize a single node from JSON.
   *
   * @param j JSON object
   * @return Unique pointer to created node, or nullptr on error
   */
  [[nodiscard]] static std::unique_ptr<Node> deserialize_node(const nlohmann::json &j);

private:

  /**
   * @brief Find pin by node ID and pin index.
   *
   * @param graph Graph to search
   * @param node_id Node ID
   * @param pin_index Pin index within node
   * @param is_output True for output pin, false for input pin
   * @return Pointer to pin, or nullptr if not found
   */
  [[nodiscard]] static Pin *find_pin(NodeGraph &graph,
                                     int node_id,
                                     size_t pin_index,
                                     bool is_output);
};

#endif  // NAG_ENGINE_SERIALIZATION_JSON_GRAPH_SERIALIZER_H

#ifndef NAG_ENGINE_SERIALIZATION_GRAPH_SERIALIZER_H
#define NAG_ENGINE_SERIALIZATION_GRAPH_SERIALIZER_H

#include <string>

#include "engine/node_graph.h"
#include "engine/node_registry.h"

/**
 * @brief Abstract interface for graph serialization.
 *
 * Allows different serialization formats (JSON, MessagePack, etc.)
 * to be implemented and swapped.
 */
class IGraphSerializer {
public:
  virtual ~IGraphSerializer() = default;

  /**
   * @brief Save a graph to file.
   *
   * @param graph Graph to save
   * @param path File path
   * @return Result indicating success or error
   */
  [[nodiscard]] virtual OperationResult save(
    const NodeGraph &graph,
    const std::string &path
  ) const = 0;

  /**
   * @brief Load a graph from file.
   *
   * @param graph Graph to populate (will be cleared first)
   * @param path File path
   * @return Result indicating success or error
   */
  [[nodiscard]] virtual OperationResult load(
    NodeGraph &graph,
    const std::string &path
  ) const = 0;
};

#endif  // NAG_ENGINE_SERIALIZATION_GRAPH_SERIALIZER_H

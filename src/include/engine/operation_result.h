#ifndef NAG_ENGINE_OPERATION_RESULT_H
#define NAG_ENGINE_OPERATION_RESULT_H

#include <string>

/**
 * @brief Result type for operations that can fail with an error message.
 */
struct OperationResult {
  bool success{false};
  std::string error_message;

  [[nodiscard]] static OperationResult ok() {
    return {true, ""};
  }

  [[nodiscard]] static OperationResult error(const std::string& msg) {
    return {false, msg};
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return success;
  }
};


#endif //NAG_ENGINE_OPERATION_RESULT_H
#ifndef NAG_ID_GENERATOR_H
#define NAG_ID_GENERATOR_H

#include <cstdint>

struct IdGenerator {
  uint32_t id = 0;
  uint32_t generate_id() { return ++id; }
};

#endif //NAG_ID_GENERATOR_H
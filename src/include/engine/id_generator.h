#ifndef NAG_ID_GENERATOR_H
#define NAG_ID_GENERATOR_H

struct IdGenerator {
  int id = 0;
  int generate_id() { return ++id; }
};

#endif //NAG_ID_GENERATOR_H
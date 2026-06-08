#ifndef NAG_ID_GENERATOR_H
#define NAG_ID_GENERATOR_H

template<typename T=int>
struct IdGenerator {
  T id = 0;
  T generate_id() { return ++id; }
};

#endif //NAG_ID_GENERATOR_H
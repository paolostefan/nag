#ifndef NAG_ENGINE_NODE_TYPE_H
#define NAG_ENGINE_NODE_TYPE_H

enum class NodeType : uint8_t {
  Default,

  // Generators
  Constant,
  Time,
  Mandel,
  Noise,
  Random,
  StepSequencer,
  ParticleEmitter,
  ParticleSystem,

  // Unary math operators
  Abs,
  Floor,
  Ceil,
  Round,
  Sqrt,
  Negate,

  // Trigonometric functions
  Sin,
  Cos,
  Tan,

  // Binary math operators
  Subtract,
  Multiply,
  Divide,
  Modulo,
  Power,
  Compare,

  // N-ary math operators
  Add,
  Min,
  Max,

  // "Special" operators
  Remap,
  Clamp,
  Lerp,
  SmoothStep,

  // Temporal modifiers
  LFO,
  Envelope,
  Delay,
  Smoother,

  // Visual nodes
  ClearColor,
  Gradient,
  Circle,
  Ellipse,
  Rectangle2D,
  Polygon,
  TextureLoader,
  Tile,
  ParticleRenderer,

  // FX nodes
  Blur,
  ChromaticAberration,
  ColorCorrection,
  Composite,
  Displace,
  Pixelate,
  SDFShape,
  Transform,
  VintageCRT,

  // Sink
  Output,

  Count,
};


#endif //NAG_ENGINE_NODE_TYPE_H

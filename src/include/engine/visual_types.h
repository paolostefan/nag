#ifndef NAG_VISUAL_TYPES_H
#define NAG_VISUAL_TYPES_H

#include "imgui.h"

struct Vec2
{
  float x{0.f};
  float y{0.f};

  Vec2(const float _x, const float _y) : x(_x), y(_y) {}
};

struct Vec3
{
  float x{0.f};
  float y{0.f};
  float z{0.f};

  Vec3(const float _x, const float _y, const float _z) : x(_x), y(_y), z(_z) {}
};

/**
 * 4D vector, also used for RGBA colors
 */
struct Vec4
{
  float x{0.f};
  float y{0.f};
  float z{0.f};
  float w{0.f};

  Vec4() = default;

  // w = 1.f by default for colors (alpha = 1.f)
  Vec4(const float _x, const float _y, const float _z, const float _w=1.f) : x(_x), y(_y), z(_z), w(_w) {}

  explicit Vec4(const ImVec4 &im_vec4) : x(im_vec4.x), y(im_vec4.y), z(im_vec4.z), w(im_vec4.w) {}

  Vec4& operator=(const ImVec4& im_vec4) {
    x = im_vec4.x;
    y = im_vec4.y;
    z = im_vec4.z;
    w = im_vec4.w;
    return *this;
  }

  // Color aliases
  [[nodiscard]] constexpr float r() const { return x; }
  [[nodiscard]] constexpr float g() const { return y; }
  [[nodiscard]] constexpr float b() const { return z; }
  [[nodiscard]] constexpr float a() const { return w; }

  // Common colors
  static Vec4 black() { return Vec4(0.f, 0.f, 0.f); }
  static Vec4 white() { return Vec4(1.f, 1.f, 1.f); }
  static Vec4 red() { return Vec4(1.f, 0.f, 0.f); }
  static Vec4 green() { return Vec4(0.f, 1.f, 0.f); }
  static Vec4 blue() { return Vec4(0.f, 0.f, 1.f); }
  static Vec4 yellow() { return Vec4(1.f, 1.f, 0.f); }
  static Vec4 cyan() { return Vec4(0.f, 1.f, 1.f); }
  static Vec4 magenta() { return Vec4(1.f, 0.f, 1.f); }
  static Vec4 orange() { return Vec4(1.f, 0.647f, 0.f); }
  static Vec4 purple() { return Vec4(0.5f, 0.f, 0.5f); }
  static Vec4 gray() { return Vec4(0.5f, 0.5f, 0.5f); }
  static Vec4 transparent() { return Vec4(0.f, 0.f, 0.f, 0.f); }
};

using ColorRGB = Vec3;
using Color = Vec4;

#endif //NAG_VISUAL_TYPES_H
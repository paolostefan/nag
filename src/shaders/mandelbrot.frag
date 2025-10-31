#version 150 core

in vec2 tex_coord;

out vec4 frag_color;

uniform sampler2D tex;
uniform float time;

void main()
{
  vec2 uv = tex_coord;
  vec2 c = vec2(0.0, 0.0);
  float r = 0.0;
  float g = 0.0;
  float b = 0.0;
  float a = 0.0;

  for (int i = 0; i < 100; i++)
  {
    r = r * r - g * g + c.x;
    g = r * r + c.y;
    b = r * r - g * g + c.y;
    a = r * r + g * g + b * b;
    c = vec2(a, b);
  }

  frag_color = vec4(r, g, b, 1.0);
}
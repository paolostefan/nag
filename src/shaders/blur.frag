#version 330 core

// Separable Gaussian blur — one pass per axis.
// Called twice by BlurNode::render():
//   Pass 1: u_horizontal = 1  →  blur along X, output to aux_target
//   Pass 2: u_horizontal = 0  →  blur along Y, output to render_target

in  vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform float     u_radius;       // blur radius in pixels
uniform vec2      u_resolution;
uniform int       u_horizontal;   // 1 = horizontal, 0 = vertical

// 9-tap Gaussian weights (sigma ≈ radius/2).
// Weights are normalised so they sum to 1.
const int   kTaps    = 9;
const float kWeights[9] = float[](
    0.0162, 0.0540, 0.1216, 0.1945, 0.2270, 0.1945,  0.1216, 0.0540, 0.0162
);

void main() {
    vec2 texel = 1.0 / u_resolution;
    vec2 dir   = u_horizontal == 1 ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

    vec4 result = vec4(0.0);
    float step_size = max(u_radius / float(kTaps / 2), 1.0);

    for (int i = 0; i < kTaps; ++i) {
        float offset = (float(i) - float(kTaps / 2)) * step_size;
        vec2  uv     = v_texcoord + dir * offset * texel;
        result      += texture(u_texture_0, uv) * kWeights[i];
    }

    frag_color = result;
}

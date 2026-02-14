#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2 u_resolution;
uniform int u_gradient_type; // 0 => linear, 1 => radial
uniform vec4 u_color_start;
uniform vec4 u_color_end;
uniform vec2 u_direction; // for linear
uniform vec2 u_center; // for radial


void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    float t = 0;

    if (u_gradient_type == 0) {
        // Linear gradient
        vec2 dir = normalize(u_direction);
        t = 0.5 + dot(uv - 0.5, dir);
    } else {
        float dist = distance(uv, u_center);
        t = dist / 0.7071; // Normalize to [0, 1] for screen diagonal
    }

    t = clamp(t, 0.0, 1.0);
    frag_color = mix(u_color_start, u_color_end, t);
}
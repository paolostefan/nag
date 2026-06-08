#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2 u_resolution;
uniform vec2 u_position;
uniform vec2 u_size;
uniform float u_rotation;
uniform vec4 u_color;
uniform float u_corner_radius;

// SDF for rounded rectangle
float rounded_box_sdf(vec2 p, vec2 b, float r) {
    vec2 d = abs(p) - b + vec2(r);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
}

void main() {
    vec2 uv = v_texcoord;

    // Transform to rectangle space
    vec2 p = uv - u_position;

    // Apply rotation
    float c = cos(u_rotation);
    float s = sin(u_rotation);
    mat2 rot = mat2(c, -s, s, c);
    p = rot * p;

    // Calculate SDF
    float dist = rounded_box_sdf(p, u_size * 0.5, u_corner_radius);

    float alpha = 1.0 - smoothstep(-0.001, 0.001, dist);

    frag_color = vec4(u_color.rgb, u_color.a * alpha);
}
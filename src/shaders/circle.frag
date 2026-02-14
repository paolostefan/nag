#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2 u_resolution;
uniform vec2 u_position;
uniform float u_radius;
uniform vec4 u_color;
uniform float u_edge_smoothness;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    float dist = distance(uv, u_position);
    float alpha = 1.0 - smoothstep(u_radius - u_edge_smoothness,
                                   u_radius + u_edge_smoothness,
                                   dist);

    frag_color = vec4(u_color.rgb, u_color.a * alpha);
}
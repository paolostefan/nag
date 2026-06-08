#version 330 core

// Chromatic aberration — offsets R and B channels along X by ±strength pixels.

in  vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform float     u_strength;    // pixel offset per channel
uniform vec2      u_resolution;

void main() {
    vec2 offset = vec2(u_strength / u_resolution.x, 0.0);

    float r = texture(u_texture_0, v_texcoord + offset).r;
    float g = texture(u_texture_0, v_texcoord         ).g;
    float b = texture(u_texture_0, v_texcoord - offset).b;
    float a = texture(u_texture_0, v_texcoord         ).a;

    frag_color = vec4(r, g, b, a);
}

#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2 u_resolution;
uniform sampler2D u_texture_0; // Base texture
uniform sampler2D u_texture_1; // Blend texture
uniform int u_blend_mode; // 0=Normal, 1=Add, 2=Multiply, 3=Screen
uniform float u_opacity;

void main() {
    vec4 base = texture(u_texture_0, v_texcoord);
    vec4 blend = texture(u_texture_1, v_texcoord);

    vec4 result = base;

    if (u_blend_mode == 0) {
        // Normal (alpha blend)
        result = mix(base, blend, blend.a * u_opacity);
    } else if (u_blend_mode == 1) {
        // Add
        result = base + blend * u_opacity;
        result.a = max(base.a, blend.a);
    } else if (u_blend_mode == 2) {
        // Multiply
        result.rgb = base.rgb * mix(vec3(1.0), blend.rgb, u_opacity);
        result.a = base.a;
    } else if (u_blend_mode == 3) {
        // Screen
        vec3 screen = vec3(1.0) - (vec3(1.0) - base.rgb) * (vec3(1.0) - blend.rgb);
        result.rgb = mix(base.rgb, screen, u_opacity);
        result.a = base.a;
    }

    frag_color = result;
}
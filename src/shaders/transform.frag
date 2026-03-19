#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform vec2  u_resolution;
uniform float u_translate_x;  // normalised [0,1] space
uniform float u_translate_y;
uniform float u_scale;         // uniform scale (1.0 = original size)
uniform float u_rotation;      // radians

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    // Transform UV around centre (0.5, 0.5)
    vec2 p = uv - vec2(0.5);

    // Inverse rotation (to find which texel maps here)
    float c = cos(-u_rotation);
    float s = sin(-u_rotation);
    p = vec2(c * p.x - s * p.y,
             s * p.x + c * p.y);

    // Inverse scale
    float sc = max(u_scale, 0.0001);
    p /= sc;

    // Inverse translation
    p -= vec2(u_translate_x, u_translate_y);

    // Back to [0,1] UV
    vec2 sample_uv = p + vec2(0.5);

    // Discard fragments outside texture bounds → transparent
    if (sample_uv.x < 0.0 || sample_uv.x > 1.0 ||
        sample_uv.y < 0.0 || sample_uv.y > 1.0) {
        frag_color = vec4(0.0);
        return;
    }

    frag_color = texture(u_texture_0, sample_uv);
}

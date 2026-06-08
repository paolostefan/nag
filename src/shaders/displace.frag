#version 330 core

in  vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;   // source image
uniform sampler2D u_texture_1;   // displacement map
uniform vec2      u_resolution;
uniform float     u_strength;    // displacement amount in UV units [0, 0.5]
uniform float     u_channel_x;   // map channel used for X displacement: 0=R 1=G 2=B
uniform float     u_channel_y;   // map channel used for Y displacement: 0=R 1=G 2=B

float sample_channel(vec4 color, float ch) {
    int c = int(round(ch));
    if (c == 0) return color.r;
    if (c == 1) return color.g;
    return color.b;
}

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    // Sample displacement map, remap [0,1] → [-1,1]
    vec4 map   = texture(u_texture_1, uv);
    float dx   = (sample_channel(map, u_channel_x) - 0.5) * 2.0 * u_strength;
    float dy   = (sample_channel(map, u_channel_y) - 0.5) * 2.0 * u_strength;

    vec2 displaced = uv + vec2(dx, dy);

    frag_color = texture(u_texture_0, displaced);
}

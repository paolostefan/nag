#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform vec2 u_resolution;
uniform float u_tile_x;     // repetitions along X [1, N]
uniform float u_tile_y;     // repetitions along Y [1, N]
uniform float u_offset_x;   // UV scroll offset X
uniform float u_offset_y;   // UV scroll offset Y
uniform float u_mirror_x;   // 1.0 = mirror along X, 0.0 = repeat
uniform float u_mirror_y;   // 1.0 = mirror along Y, 0.0 = repeat

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution;

    // Apply tiling and offset
    vec2 tiled = uv * vec2(u_tile_x, u_tile_y)
    + vec2(u_offset_x, u_offset_y);

    // Mirror or repeat per axis
    // fract(t) gives [0,1) repeat.
    // For mirror: fract of the integer part selects even/odd tile,
    // then flip uv within the tile if odd.
    vec2 t = fract(tiled);

    if (u_mirror_x > 0.5) {
        float tile_idx = floor(tiled.x);
        if (mod(tile_idx, 2.0) != 0.0) t.x = 1.0 - t.x;
    }

    if (u_mirror_y > 0.5) {
        float tile_idx = floor(tiled.y);
        if (mod(tile_idx, 2.0) != 0.0) t.y = 1.0 - t.y;
    }

    frag_color = texture(u_texture_0, t);
}

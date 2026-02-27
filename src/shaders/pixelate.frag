#version 330 core

// Pixelate / mosaic — quantises UVs to blocks of pixel_size x pixel_size.

in  vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform float     u_pixel_size;   // block size in pixels
uniform vec2      u_resolution;

void main() {
    // Clamp pixel_size to avoid division by zero.
    float ps = max(u_pixel_size, 1.0);

    // Convert UV → pixel coords → snap to grid → back to UV.
    vec2 pixel_coords  = v_texcoord * u_resolution;
    vec2 snapped       = floor(pixel_coords / ps) * ps + ps * 0.5;
    vec2 snapped_uv    = snapped / u_resolution;

    frag_color = texture(u_texture_0, snapped_uv);
}

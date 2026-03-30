#version 330 core

// Vintage CRT effect — quantises UVs to blocks of pixel_size x pixel_size.
// Adds a slight blur to the pixelated image to simulate the glow of a CRT display.
// Moreover, adds scanlines by darkening every other row of pixels.

in vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform float u_pixel_size;   // block size in pixels
uniform vec2 u_resolution;

void main() {

    const float kScanlineMaxIntensity = 0.41; // Maximum intensity of scanlines (0.0 to 1.0)

    // Clamp pixel_size to avoid division by zero.
    float ps = max(u_pixel_size, 1.0);

    // Convert UV → pixel coords → snap to grid → back to UV.
    vec2 pixel_coords = v_texcoord * u_resolution;
    vec2 snapped = floor(pixel_coords / ps) * ps + ps * 0.5;
    vec2 snapped_uv = snapped / u_resolution;

    vec4 pixel_color = texture(u_texture_0, snapped_uv);

    // Add a slight blur by sampling the neighboring pixels and averaging.
    vec4 blur_color = (
    texture(u_texture_0, snapped_uv + vec2(-ps, 0.0) / u_resolution) +
    texture(u_texture_0, snapped_uv + vec2(ps, 0.0) / u_resolution) +
    texture(u_texture_0, snapped_uv + vec2(0.0, -ps) / u_resolution) +
    texture(u_texture_0, snapped_uv + vec2(0.0, ps) / u_resolution)
    ) * 0.25;

    // Combine the original pixel color with the blur color.
    vec4 combined_color = mix(pixel_color, blur_color, 0.5);

    // Add scanlines by oscillating
    float apply_scanline = abs(sin(pixel_coords.y * 2.0 / ps)) * kScanlineMaxIntensity;
    combined_color = mix(combined_color, vec4(0.0, 0.0, 0.0, 1.0), apply_scanline);

    frag_color = combined_color;
}

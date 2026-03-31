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

    const float kPi = 3.1415926535897932384626433832795;
    const float kScanlineMaxIntensity = 0.51; // Maximum intensity of scanlines [0..1] with 0: no scanlines, 1: full black scanlines.

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

    // Add random color jitter to simulate the slight color instability of CRTs.
    vec3 jitter_amount = vec3(0.07, 0.02, 0.03); // Adjust this value for more or less jitter.
    vec3 jitter = vec3(
    sin(dot(snapped, vec2(129.897, -78.233)) * 4307.354111119),
    sin(dot(snapped, vec2(939.897, -67.345)) * 7771.65452),
    sin(dot(snapped, vec2(-453.321, 23.456)) * 10147.79791)
    );

    pixel_color.rgb += jitter * jitter_amount;

    // Combine the original pixel color with the blur color.
    vec4 combined_color = mix(pixel_color, blur_color, 0.5);

    // Add scanlines by oscillating brightness based on the y-coordinate of the pixel.
    float apply_scanline = cos(pixel_coords.y * kPi / ps);
    combined_color = mix(combined_color, vec4(0.0, 0.0, 0.0, 1.0),
                         apply_scanline * apply_scanline * kScanlineMaxIntensity);

    frag_color = combined_color;
}

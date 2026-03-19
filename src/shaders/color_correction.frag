#version 330 core

in  vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0;
uniform vec2      u_resolution;
uniform float     u_brightness;  // additive offset  [-1, 1], 0 = no change
uniform float     u_contrast;    // multiplier       [ 0, 4], 1 = no change
uniform float     u_saturation;  // 0 = greyscale,   1 = original,  >1 = boost
uniform float     u_hue_shift;   // rotation in radians [0, 2π]

// Rotate hue by angle (radians) in RGB space using Rodrigues' formula.
vec3 rotate_hue(vec3 rgb, float angle) {
    // Axis aligned with the grey diagonal (1,1,1)/sqrt(3)
    float c = cos(angle);
    float s = sin(angle);
    float k = 1.0 / 3.0;            // 1 - cos(120°) component

    // Rotation matrix rows
    vec3 r0 = vec3(c + k*(1.0-c),     k*(1.0-c) - s*0.5774, k*(1.0-c) + s*0.5774);
    vec3 r1 = vec3(k*(1.0-c) + s*0.5774, c + k*(1.0-c),     k*(1.0-c) - s*0.5774);
    vec3 r2 = vec3(k*(1.0-c) - s*0.5774, k*(1.0-c) + s*0.5774, c + k*(1.0-c)    );

    return clamp(vec3(dot(rgb, r0), dot(rgb, r1), dot(rgb, r2)), 0.0, 1.0);
}

void main() {
    vec2 uv    = gl_FragCoord.xy / u_resolution;
    vec4 color = texture(u_texture_0, uv);

    // ── Brightness (additive) ─────────────────────────────────────────────
    color.rgb += u_brightness;

    // ── Contrast (pivot at 0.5) ───────────────────────────────────────────
    color.rgb = (color.rgb - 0.5) * u_contrast + 0.5;

    // ── Saturation ────────────────────────────────────────────────────────
    float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    color.rgb  = mix(vec3(luma), color.rgb, u_saturation);

    // ── Hue shift ─────────────────────────────────────────────────────────
    if (abs(u_hue_shift) > 0.001) {
        color.rgb = rotate_hue(color.rgb, u_hue_shift);
    }

    frag_color = vec4(clamp(color.rgb, 0.0, 1.0), color.a);
}

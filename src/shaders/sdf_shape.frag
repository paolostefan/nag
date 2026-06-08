#version 330 core

in  vec2 v_texcoord;
out vec4 frag_color;

uniform vec2  u_resolution;
uniform vec2  u_position;
uniform float u_radius;          // circumradius / half-extent (fraction of screen height)
uniform float u_rotation;        // radians
uniform float u_shape;           // 0 = circle, 1 = box, 2 = ring
uniform float u_aspect;          // box only: width/height ratio (1.0 = square)
uniform float u_ring_thickness;  // ring only: normalised thickness [0, 1]
uniform vec4  u_color;
uniform float u_edge_smoothness;

// ── SDF functions ─────────────────────────────────────────────────────────────

float sdf_circle(vec2 p, float r) {
    return length(p) - r;
}

// Axis-aligned box, half-extents b
float sdf_box(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdf_ring(vec2 p, float r, float thickness) {
    return abs(length(p) - r) - thickness * r;
}

void main() {
    float aspect = u_resolution.x / u_resolution.y;

    // Aspect-corrected isotropic space, centred at origin
    vec2 uv  = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspect, 1.0);
    vec2 pos = (u_position                     - 0.5) * vec2(aspect, 1.0);

    vec2 p = uv - pos;

    // Rotate into shape-local frame
    float c = cos(-u_rotation);
    float s = sin(-u_rotation);
    p = vec2(c * p.x - s * p.y,
             s * p.x + c * p.y);

    // Select SDF by shape index
    float dist;
    int shape = int(round(u_shape));

    if (shape == 1) {
        // Box: half-extents derived from radius and aspect ratio
        vec2 half_ext = vec2(u_radius * u_aspect, u_radius);
        dist = sdf_box(p, half_ext);
    } else if (shape == 2) {
        // Ring
        dist = sdf_ring(p, u_radius, u_ring_thickness);
    } else {
        // Circle (default)
        dist = sdf_circle(p, u_radius);
    }

    float alpha = 1.0 - smoothstep(-u_edge_smoothness,
                                    u_edge_smoothness,
                                    dist);

    frag_color = vec4(u_color.rgb, u_color.a * alpha);
}

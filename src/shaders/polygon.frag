#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2  u_resolution;
uniform vec2  u_position;
uniform float u_radius;
uniform float u_n_sides;    // passed as float, used via int cast internally
uniform float u_rotation;   // radians
uniform vec4  u_color;
uniform float u_edge_smoothness;

// Signed distance to a regular n-gon centred at origin, circumradius r.
// Adapted from Inigo Quilez: https://iquilezles.org/articles/distfunctions2d/
float sdf_polygon(vec2 p, float r, float n) {
    float an = 3.141593 / n;                 // half internal angle
    float he = r * tan(an);                  // half-edge length

    // Reduce to fundamental sector
    float bn = mod(atan(p.x, p.y), 2.0 * an) - an;
    vec2 q = length(p) * vec2(cos(bn), abs(sin(bn)));

    // Distance to the nearest edge
    q -= vec2(r, 0.0);
    q.y += clamp(-q.y, 0.0, he);
    return length(q) * sign(q.x);
}

void main() {
    float aspect = u_resolution.x / u_resolution.y;

    // Aspect-corrected space: centre at origin, X scaled by aspect.
    vec2 uv  = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspect, 1.0);
    vec2 pos = (u_position                     - 0.5) * vec2(aspect, 1.0);

    vec2 p = uv - pos;

    // Rotate
    float c = cos(-u_rotation);
    float s = sin(-u_rotation);
    p = vec2(c * p.x - s * p.y,
             s * p.x + c * p.y);

    float n    = max(u_n_sides, 3.0);
    float dist = sdf_polygon(p, u_radius, n);

    float alpha = 1.0 - smoothstep(-u_edge_smoothness,
                                    u_edge_smoothness,
                                    dist);

    frag_color = vec4(u_color.rgb, u_color.a * alpha);
}

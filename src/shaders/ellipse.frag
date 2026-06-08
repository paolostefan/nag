#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2  u_resolution;
uniform vec2  u_position;
uniform float u_radius_x;
uniform float u_radius_y;
uniform float u_rotation;   // radians
uniform vec4  u_color;
uniform float u_edge_smoothness;

void main() {
    float aspect = u_resolution.x / u_resolution.y;

    // Aspect-corrected space: centre at origin, X scaled by aspect.
    vec2 uv  = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspect, 1.0);
    vec2 pos = (u_position                     - 0.5) * vec2(aspect, 1.0);

    // Vector from ellipse centre to fragment
    vec2 p = uv - pos;

    // Rotate into ellipse-local frame
    float c = cos(-u_rotation);
    float s = sin(-u_rotation);
    p = vec2(c * p.x - s * p.y,
             s * p.x + c * p.y);

    // Normalised ellipse SDF (inside when dist < 1)
    float dist  = length(vec2(p.x / max(u_radius_x, 0.0001),
                              p.y / max(u_radius_y, 0.0001)));

    float alpha = 1.0 - smoothstep(1.0 - u_edge_smoothness,
                                   1.0 + u_edge_smoothness,
                                   dist);

    frag_color = vec4(u_color.rgb, u_color.a * alpha);
}

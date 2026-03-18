#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform vec2  u_resolution;
uniform vec2  u_position;
uniform float u_radius;
uniform vec4  u_color;
uniform float u_edge_smoothness;

void main() {
    float aspect = u_resolution.x / u_resolution.y;

    // Aspect-corrected space: centre at origin, X scaled by aspect.
    // Distances are now isotropic (1 unit = 1 "screen height unit").
    vec2 uv  = (gl_FragCoord.xy / u_resolution - 0.5) * vec2(aspect, 1.0);
    vec2 pos = (u_position                     - 0.5) * vec2(aspect, 1.0);

    float dist  = distance(uv, pos);
    float alpha = 1.0 - smoothstep(u_radius - u_edge_smoothness,
                                   u_radius + u_edge_smoothness,
                                   dist);

    frag_color = vec4(u_color.rgb, u_color.a * alpha);
}

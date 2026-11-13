#version 330 core

in vec2 v_texcoord;
out vec4 frag_color;

uniform sampler2D u_texture_0; // source image
uniform sampler2D u_texture_1; // height map (greyscale)
uniform vec2 u_resolution;

uniform float u_light_x; // -1..1
uniform float u_light_y; // -1..1
uniform float u_strength; // height exaggeration
uniform float u_specular; // glossiness

void main() {
    vec2 uv = v_texcoord;

    // ── 1. Height gradient, one texel at a time ──────────────────────────
    // textureSize works even when the height map is not the render size.
    vec2 texel = 1.0 / vec2(textureSize(u_texture_1, 0));

    float h_l = texture(u_texture_1, uv - vec2(texel.x, 0.0)).r;
    float h_r = texture(u_texture_1, uv + vec2(texel.x, 0.0)).r;
    float h_d = texture(u_texture_1, uv - vec2(0.0, texel.y)).r;
    float h_u = texture(u_texture_1, uv + vec2(0.0, texel.y)).r;

    vec2 dH = vec2(h_r - h_l, h_u - h_d) * 0.5; // delta height

    // ── 2. Height gradient → normal ──────────────────────────────────────
    vec3 n = normalize(vec3(-dH * u_strength, 1.0));

    // ── 3. Two pins + fixed z → light direction ──────────────────────────
    vec3 l = normalize(vec3(u_light_x, u_light_y, 1.0));

    // ── 4. Diffuse + Blinn-Phong specular ────────────────────────────────
    float diffuse = max(dot(n, l), 0.0);

    vec3 view_dir = vec3(0.0, 0.0, 1.0);
    vec3 half_dir = normalize(l + view_dir);
    float spec = pow(max(dot(n, half_dir), 0.0), 32.0) * u_specular;

    // ── 5. Re-light the source, keep its alpha ───────────────────────────
    vec4 src = texture(u_texture_0, uv);
    vec3 shaded = src.rgb * (0.25+0.75*diffuse)+vec3(spec);

    frag_color = vec4(shaded, src.a);
}
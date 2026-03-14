#version 330 core

// Generates a fullscreen quad from gl_VertexID — no VBO needed.
// Vertices:  0,1,2  (lower-left triangle)
//            3,4,5  (upper-right triangle)
out vec2 v_texcoord;

void main() {
    // Two-triangle strip covering clip space [-1, 1].
    const vec2 kPositions[6] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0, -1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  1.0)
    );

    vec2 pos = kPositions[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);

    // Map [-1,1] → [0,1] and flip Y: OpenGL FBO origin is bottom-left,
    // but we want top-left to match screen conventions.
    v_texcoord = vec2(pos.x * 0.5 + 0.5, 1.0 - (pos.y * 0.5 + 0.5));
}

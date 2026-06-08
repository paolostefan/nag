#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;

uniform vec2 uResolution;
uniform float uGlobalScale;

out vec4 vColor;

void main() {
    vec2 pos = aPos * uGlobalScale;
    vec2 ndc = pos / uResolution * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = aSize;
    vColor = aColor;
}

#version 330 core
in vec4 vColor;
uniform sampler2D uSpriteTexture;
out vec4 FragColor;

void main() {
    vec4 texColor = texture(uSpriteTexture, gl_PointCoord);
    FragColor = vec4(texColor.rgb * vColor.rgb, texColor.a * vColor.a);
}

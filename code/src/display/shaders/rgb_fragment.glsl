#version 450 core
in vec2 texCoord;
in float alpha;
uniform sampler2D rgbTexture;
out vec4 fragColor;
void main()
{
    vec4 rgb = texture(rgbTexture,texCoord);
    fragColor = vec4(rgb.r,rgb.g,rgb.b,alpha);
}
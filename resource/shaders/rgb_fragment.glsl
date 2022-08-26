#version 450 core
in vec2 texCoord;
in float alpha;
uniform sampler2D texture;
out vec4 fragColor;
void main()
{
    vec3 rgb = texture(texture,texCoord);
    fragColor = vec4(rgb,alpha);
}
#version 450 core
in vec2 texCoord;
in float alpha;
uniform sampler2D yTexture;
uniform sampler2D uvTexture;
out vec4 fragColor;
void main()
{
    vec4 yColor = texture(yTexture,texCoord);
    vec4 uvColor = texture(uvTexture,texCoord)-0.5;
    vec3 yuv = vec3(yColor.r,uvColor.g,uvColor.a);
    mat3 cvt = mat3(1,       1,         1, 
                    0,       -0.39465,  2.03211, 
                    1.13983, -0.58060,  0);
    vec3 rgb = cvt*yuv;
    fragColor = vec4(rgb,alpha);
}
#version 450
precision highp float;

in vec2 texCoord;
out vec4 fragmentColor;

uniform sampler2D image;

void main()
{
    fragmentColor = texture(image, texCoord);
}
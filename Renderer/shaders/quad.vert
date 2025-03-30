#version 450				
precision highp float;

layout(location=0) in vec4 vertexPosition;
layout(location=1) in vec4 vertexNormal;
layout(location=2) in vec2 vertexTexCoord;

out vec2 texCoord;
out vec4 rayDir;

uniform struct {
  mat4 rayDirMatrix;
  vec3 position;
  vec3 ahead;
} camera;


void main(void) {
  texCoord = vertexTexCoord;
  gl_Position = vertexPosition;
  gl_Position.z = 0.9999999;
  rayDir = vertexPosition * camera.rayDirMatrix;
}
#version 450

layout (binding = 0) uniform Matrices {
  mat4 mvp;
};

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Color;

layout (location = 0) out vec3 out_Colour;

void main() {
  gl_Position = vec4(in_Position, 1.0f);
  out_Colour = in_Color;
}

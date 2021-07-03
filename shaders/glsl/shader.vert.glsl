#version 450

layout (binding = 0) uniform Matrices {
  mat4 mvp;
};

layout (binding = 1) uniform sampler2D tex_Sampler;

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Color;
layout (location = 2) in vec2 in_TexCoord;

layout (location = 0) out vec3 out_Colour;
layout (location = 1) out vec2 out_TexCoord;

void main() {
  gl_Position = mvp * vec4(in_Position, 1.0f);
  out_Colour = in_Color;
  out_TexCoord = in_TexCoord;
}

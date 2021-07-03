#version 450

layout (binding = 1) uniform sampler2D tex_Sampler;

layout (location = 0) in vec3 in_Colour;
layout (location = 1) in vec2 in_TexCoord;

layout (location = 0) out vec4 out_Colour;

void main() {
  vec4 tex = texture(tex_Sampler, in_TexCoord);
  out_Colour = tex * vec4(in_Colour, 1.0f);
}

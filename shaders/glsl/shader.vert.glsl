#version 450

const vec3 vertices[] = {
  vec3(0.0f, 0.5, 0.0f),
  vec3(-0.5f, -0.5f, 0.0f),
  vec3(0.5f, -0.5f, 0.0f)
};

void main() {
  gl_Position = vec4(vertices[gl_VertexIndex], 1.0f);
}

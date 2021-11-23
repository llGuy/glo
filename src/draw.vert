#version 330 core

out vec2 fragCoord;

const vec2 POSITIONS[4] = vec2[4](
  vec2(-1.0), vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0));

void main() {
  fragCoord = POSITIONS[gl_VertexID];
  gl_Position = vec4(POSITIONS[gl_VertexID], 0.0, 1.0);
}

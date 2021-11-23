#version 330 core

in vec2 fragCoord;

out vec4 outFragColor;

layout (std140) uniform SceneData {
  mat4 invOrtho;
  vec2 wCirclePosition;
  float wCircleRadius;
} uSceneData;

///////////////////////////////////////////////////////////////////////////////
//           A bunch of SDF functions used in the rendering process          //
///////////////////////////////////////////////////////////////////////////////
float sdCircle(vec2 p, float r) {
  return length(p) - r;
}

float sdSegment(vec2 p, vec2 a, vec2 b) {
  vec2 pa = p-a, ba = b-a;
  float h = clamp(dot(pa, ba)/dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

///////////////////////////////////////////////////////////////////////////////
//                                 Rendering                                 //
///////////////////////////////////////////////////////////////////////////////
float map(in vec2 wCoord) {
  float d = 1e10;

  // Testing circle
  d = min(d, sdCircle(
               wCoord - uSceneData.wCirclePosition,
               uSceneData.wCircleRadius));

  // Grid

  return d;
}

void main() {
  vec2 wCoord = (uSceneData.invOrtho * vec4(fragCoord,0.0,1.0)).xy;
  float d = map(wCoord);

  if (d < 0.0) {
    outFragColor = vec4(1.0);
  }
  else {
    outFragColor = vec4(0.0);
  }
}

#version 330 core

in vec2 fragCoord;

out vec4 outFragColor;

layout (std140) uniform SceneData {
  mat4 invOrtho;
  vec2 wMapStart;
  vec2 wMapEnd;
  float wGridScale;
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
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h );
}

///////////////////////////////////////////////////////////////////////////////
//                                 Rendering                                 //
///////////////////////////////////////////////////////////////////////////////
float mapObjects(in vec2 wCoord) {
  float d = 1e10;

  // Testing circle
  d = min(d, sdCircle(
            wCoord - uSceneData.wCirclePosition,
            uSceneData.wCircleRadius));

  return d;
}

// Grid mapping
float mapGrid(vec2 wCoord) {
  float d = 1e10;

  vec2 clamped = clamp(
    wCoord,
    // We need to subtract and add 0.5 to stop the grid from going forever
    uSceneData.wMapStart-vec2(0.5),
    uSceneData.wMapEnd+vec2(0.5));

  // Vertical bars
  d = min(d, sdSegment(
            mod(clamped, vec2(uSceneData.wGridScale, 0.0)),
            vec2(0.0, uSceneData.wMapStart.y),
            vec2(0.0, uSceneData.wMapEnd.y))-0.05);

  // Horizontal bars
  d = min(d, sdSegment(
            mod(clamped, vec2(0.0, uSceneData.wGridScale)),
            vec2(uSceneData.wMapStart.x, 0.0),
            vec2(uSceneData.wMapEnd.x, 0.0))-0.05);

  return d;
}

void main() {
  vec2 wCoord = (uSceneData.invOrtho * vec4(fragCoord,0.0,1.0)).xy;
  float d = mapObjects(wCoord);

  if (d <= 0.0001) {
    outFragColor = vec4(1.0);
  }
  else {
    float gridD = mapGrid(wCoord);

    if (gridD <= 0.0001) {
      outFragColor = vec4(0.1, 0.1, 0.1, 1.0);
    }
    else {
      outFragColor = vec4(0.0);
    }
  }
}

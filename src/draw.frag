#version 330 core

in vec2 fragCoord;

out vec4 outFragColor;

layout (std140) uniform SceneData {
  mat4 invOrtho;
  vec2 wMapStart;
  vec2 wMapEnd;
  float wGridScale;

  // x,y coordinates; z=orient; w=scale
  vec4 wPlayerProp;
} uSceneData;

///////////////////////////////////////////////////////////////////////////////
//                                 Math stuff                                //
///////////////////////////////////////////////////////////////////////////////
mat2 rotate(float angle) {
  mat2 r = mat2(
    cos(angle), sin(angle),
    -sin(angle), cos(angle));
  return r;
}

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

float sdTriangleIsosceles(vec2 p, vec2 q) {
  p.x = abs(p.x);
  vec2 a = p - q*clamp( dot(p,q)/dot(q,q), 0.0, 1.0 );
  vec2 b = p - q*vec2( clamp( p.x/q.x, 0.0, 1.0 ), 1.0 );
  float s = -sign( q.y );
  vec2 d = min( vec2( dot(a,a), s*(p.x*q.y-p.y*q.x) ),
                vec2( dot(b,b), s*(p.y-q.y)  ));
  return -sqrt(d.x)*sign(d.y);
}

float sdUnevenCapsule(vec2 p, float r1, float r2, float h) {
  p.x = abs(p.x);
  float b = (r1-r2)/h;
  float a = sqrt(1.0-b*b);
  float k = dot(p,vec2(-b,a));
  if( k < 0.0 ) return length(p) - r1;
  if( k > a*h ) return length(p-vec2(0.0,h)) - r2;
  return dot(p, vec2(a,b) ) - r1;
}

///////////////////////////////////////////////////////////////////////////////
//                                 Rendering                                 //
///////////////////////////////////////////////////////////////////////////////
float mapObjects(in vec2 wCoord) {
  float d = 1e10;

  // Testing circle
  d = min(d, 0.3*uSceneData.wPlayerProp.w * sdUnevenCapsule(
            (rotate(uSceneData.wPlayerProp.z) *
              (wCoord - uSceneData.wPlayerProp.xy)) / (0.3*uSceneData.wPlayerProp.w),
            0.9f, 0.2f, 3.2f));

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

  outFragColor = vec4(0.0);

  outFragColor += vec4(1.0, 0.3, 0.2, 1.0) / (d*d);

  /*
  if (d <= 0.0001) {
    outFragColor = vec4(1.0, 0.3, 0.2, 1.0);
    outFragColor += 0.2*vec4(1.0, 0.3, 0.2, 1.0) / (d*d);
  }
  else*/ {
    float gridD = mapGrid(wCoord);

    if (gridD <= 0.0001) {
      outFragColor += vec4(0.1, 0.1, 0.1, 1.0);
    }
    else {
      outFragColor += vec4(0.0);
    }
  }
}

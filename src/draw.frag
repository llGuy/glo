#version 330 core

in vec2 fragCoord;

out vec4 outFragColor;

struct Trail {
  vec2 start;
  vec2 inbetween0;
  vec2 end;
  float timeStart;
};

#define MAX_PLAYERS 20
#define MAX_TRAILS 100

layout (std140) uniform SceneData {
  mat4 invOrtho;
  vec2 wMapStart;
  vec2 wMapEnd;

  float wGridScale;
  float time;
  float maxLazerTime;
  int controlledPlayer;
  int playerCount;
  int trailCount;

  Trail trails[MAX_TRAILS];

  // x,y coordinates; z=orient; w=scale
  vec4 wPlayerProp[MAX_PLAYERS];
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
float mapControlledPlayer(in vec2 wCoord) {
  float d = 1e10;

  vec4 controlled = uSceneData.wPlayerProp[uSceneData.controlledPlayer];

  // Testing circle
  d = min(d, 0.8*controlled.w * sdUnevenCapsule(
            (rotate(controlled.z) *
              (wCoord - controlled.xy)) / (0.8*controlled.w),
            0.9f, 0.2f, 3.2f));

  return d;
}

float mapHiddenPlayers(in vec2 wCoord) {
  float d = 1e10;

  int controlled = uSceneData.controlledPlayer;

  for (int i = 0; i < uSceneData.playerCount; i++) {
    if (i != controlled) {
      vec4 prop = uSceneData.wPlayerProp[i];
      d = min(d, 0.8*prop.w * sdUnevenCapsule(
                (rotate(prop.z) *
                 (wCoord - prop.xy)) / (0.8*prop.w),
                0.9f, 0.2f, 3.2f));
    }
  }


  return d;
}

float mapLazers(in vec2 wCoord) {
  float d = 1e10;

  for (int i = 0; i < uSceneData.trailCount; i++) {
    float dt = uSceneData.time - uSceneData.trails[i].timeStart;
    float progress = dt / uSceneData.maxLazerTime;

    if (progress < 1.0) {

      vec2 start = uSceneData.trails[i].start + progress *
        (uSceneData.trails[i].end - uSceneData.trails[i].start);

      d = min(d, sdSegment(
                wCoord,
                start,
                uSceneData.trails[i].end));
    }
  }

  return d;
}

vec3 lightScene(in vec2 wCoord, float hiddenPlayers) {
  const float FINAL_LIGHT_INTENSITY = 2.0;
  bool inPlayer = (hiddenPlayers <= 0.001);

  float d = 1e10;

  vec3 litColor = vec3(0.0);

  for (int i = 0; i < uSceneData.trailCount; i++) {
    float dt = uSceneData.time - uSceneData.trails[i].timeStart;
    float progress = dt / uSceneData.maxLazerTime;

    if (progress < 1.0) {
      vec2 start = uSceneData.trails[i].start + progress *
        (uSceneData.trails[i].end - uSceneData.trails[i].start);

      d = min(d, sdSegment(
                wCoord,
                start,
                uSceneData.trails[i].end));

      vec2 diff = uSceneData.trails[i].end - wCoord;

      litColor += FINAL_LIGHT_INTENSITY*progress*vec3(1.5, 1.3, 0.5)/dot(diff,diff);
    }
    else {
      vec2 diff = uSceneData.trails[i].end - wCoord;
      litColor += FINAL_LIGHT_INTENSITY*vec3(1.5, 1.3, 0.5)/dot(diff,diff);
    }
  }

  // Get light contribution from lazer beams
  litColor += 0.2*vec3(1.5, 1.3, 0.5) / (d*d);

  if (dot(litColor,litColor) > 0.01 && inPlayer) {
    litColor += vec3(litColor) * 5.0;
  }

  return litColor;
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

vec3 calcFinalColor(vec3 color, float exposure) {
  vec3 one = vec3(1.0);
  vec3 expValue = exp(-color / vec3(1.0) * exposure);
  vec3 diff = one - expValue;
  vec3 gamma = vec3(1.0 / 2.2);
  return pow(diff, gamma);
}

void main() {
  vec2 wCoord = (uSceneData.invOrtho * vec4(fragCoord,0.0,1.0)).xy;

  // Get controlled player SDF
  float controlledPlayer = mapControlledPlayer(wCoord);

  // Get other players's SDF
  float hiddenPlayers = mapHiddenPlayers(wCoord);

  // Lazers SDF (these contribute to lighting)
  outFragColor = vec4(lightScene(wCoord, hiddenPlayers),1.0);

  if (controlledPlayer <= 0.001) {
    outFragColor.rgb += vec3(0.2);
  }

  // Grid coloring
  float gridD = mapGrid(wCoord);

  if (gridD <= 0.0001) {
    outFragColor += vec4(0.1, 0.1, 0.1, 1.0);
  }
  else {
    outFragColor += vec4(0.0);
  }


  // Gamma correction and tone mapping
  outFragColor.rgb = calcFinalColor(outFragColor.rgb, 0.06);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <GL/glew.h>

#include "io.h"
#include "glo.h"
#include "render.h"

/* Utility function to read a text file from a given path. */
static char *readTextFile(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "Unable to find file: %s\n", path);
    exit(-1);
  }

  /* Get file size */
  fseek(file, 0L, SEEK_END);
  uint32_t fileSize = ftell(file);
  char *data = malloc(sizeof(char) * (fileSize));
  rewind(file);
  fread(data, sizeof(char), fileSize, file);
  return data;
}

/* Utility function to check the status of an OpenGL shader. */
static bool checkCompileStatus(
  uint32_t shader,
  PFNGLGETSHADERIVPROC getIVProc,
  PFNGLGETSHADERINFOLOGPROC getLogProc,
  GLenum statusType) {
  int status;
  
  getIVProc(shader, statusType, &status);

  if (status != GL_TRUE) {
    fprintf(stderr, "Error compiling shader\n");
    int infoLogLength;
    getIVProc(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    char *msgBuffer = alloca(infoLogLength + 1);
    memset(msgBuffer, 0, infoLogLength + 1);
    int size;
    getLogProc(shader, infoLogLength, &size, msgBuffer);
    fprintf(stderr, "%s\n", msgBuffer);

    return false;
  }

  return true;
}

/* Utility function to compile an OpenGL shader (vertex or fragment). */
static void compileShader(uint32_t shader, const char *source) {
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  if (!checkCompileStatus(
        shader, glGetShaderiv,
        glGetShaderInfoLog, GL_COMPILE_STATUS)) {
    exit(-1);
  }
}

/* Gets called every frame to update the data the shader gets access to. */
static void updateUniformBuffer(RenderData *renderData) {
  glBindBuffer(GL_UNIFORM_BUFFER, renderData->uniformBuffer);
  GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
  memcpy(p, &renderData->uniformData, sizeof(UniformData));
  glUnmapBuffer(GL_UNIFORM_BUFFER);
}

RenderData *createRenderData(const DrawContext *ctx) {
  RenderData *renderData = (RenderData *)malloc(sizeof(RenderData));
  memset(renderData, 0, sizeof(RenderData));

  /* Create the shader which renders the scene */
  uint32_t vsh = glCreateShader(GL_VERTEX_SHADER);
  uint32_t fsh = glCreateShader(GL_FRAGMENT_SHADER);
  char *vshs = readTextFile("draw.vert");
  char *fshs = readTextFile("draw.frag");

  compileShader(vsh, vshs);
  compileShader(fsh, fshs);
  free(vshs); free(fshs);

  renderData->shader = glCreateProgram();
  glAttachShader(renderData->shader, vsh);
  glAttachShader(renderData->shader, fsh);

  glLinkProgram(renderData->shader);

  if (!checkCompileStatus(
        renderData->shader, glGetProgramiv,
        glGetProgramInfoLog, GL_LINK_STATUS)) {
    fprintf(stderr, "Unable to link shader\n");
    exit(-1);
  }

  /* Create uniform buffer in which we will store the scene information */
  renderData->uniformBlockIdx = glGetUniformBlockIndex(
    renderData->shader, "SceneData");

  static const float GRID_SIZE = 6.0f;
  renderData->uniformData.wGridScale = GRID_SIZE;
  renderData->uniformData.wMapStart = vec2(-GRID_SIZE*4.0f, -GRID_SIZE*4.0f);
  renderData->uniformData.wMapEnd = vec2(GRID_SIZE*4.0f,GRID_SIZE*4.0f);
  glGenBuffers(1, &renderData->uniformBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, renderData->uniformBuffer);
  glBufferData(
    GL_UNIFORM_BUFFER, sizeof(UniformData),
    &renderData->uniformData, GL_DYNAMIC_DRAW);

  glBindBufferBase(
    GL_UNIFORM_BUFFER, renderData->uniformBlockIdx,
    renderData->uniformBuffer);

  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  return renderData;
}

void render(
  const GloState *game,
  DrawContext *ctx,
  RenderData *renderData) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  static float wWidth = 9.0f*5.0f;

  { /* Update the uniform data with game data */
    const Player *me = &game->players[game->controlled];

    float aspect = (float)ctx->width / (float)ctx->height;
    float wHeight = wWidth / aspect;

    Vec2 mid = vec2(wWidth/2.0f, wHeight/2.0f);

    renderData->uniformData.invOrtho = invOrtho(
      vec2(me->position.x-mid.x, me->position.y-mid.y), wWidth,
      (float)ctx->width/(float)ctx->height);
    ctx->invOrtho = renderData->uniformData.invOrtho;

    renderData->uniformData.controlledPlayer = game->controlled;
    renderData->uniformData.playerCount = game->playerCount;

    for (int i = 0; i < game->playerCount; ++i) {
      const Player *p = &game->players[i];
      renderData->uniformData.wPlayerProp[i].x = p->position.x;
      renderData->uniformData.wPlayerProp[i].y = p->position.y;
      renderData->uniformData.wPlayerProp[i].z = p->orientation;
      renderData->uniformData.wPlayerProp[i].w = 0.5f;
    }

    renderData->uniformData.time = getTime();
    renderData->uniformData.maxLazerTime = MAX_LAZER_TIME;

    renderData->uniformData.bulletTrailCount = 0;
    for (int i = 0; i < game->bulletTrailCount; ++i) {
      if (getBit(&game->bulletOccupation, i)) {
        renderData->uniformData.bulletTrails[
          renderData->uniformData.bulletTrailCount] = game->bulletTrails[i];

        renderData->uniformData.bulletTrailCount++;
      }
    }
    
    updateUniformBuffer(renderData);
  }

  glUseProgram(renderData->shader);
  glBindBuffer(GL_UNIFORM_BUFFER, renderData->uniformBuffer);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

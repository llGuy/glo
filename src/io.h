#ifndef _IO_H_
#define _IO_H_

#include <stdbool.h>

#include "glo.h"
#include "math.h"

struct GLFWwindow;

typedef struct DrawContext {
  /* Stuff to do with IO and the window */
  int width, height;
  struct GLFWwindow *window;

  /* Frame rate and timing */
  double currentTime;
  float dt;

  /* For rendering */
  Mat4 invOrtho;
} DrawContext;

DrawContext *createDrawContext();
bool isContextClosed(DrawContext *ctx);
void tickDisplay(DrawContext *ctx);
GameCommands translateIO(DrawContext *ctx);

#endif

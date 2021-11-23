#ifndef _IO_H_
#define _IO_H_

#include <stdbool.h>

#include "math.h"

struct GLFWwindow;

typedef struct DrawContext {
  /* Stuff to do with IO and the window */
  int width, height;
  struct GLFWwindow *window;

  /* For rendering */
  Mat4 ortho;
} DrawContext;

DrawContext *createDrawContext();
bool isContextClosed(DrawContext *ctx);
void tickDisplay(DrawContext *ctx);

#endif

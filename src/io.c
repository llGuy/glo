#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "io.h"

DrawContext *createDrawContext() {
  DrawContext *ctx = (DrawContext *)malloc(sizeof(DrawContext));

  /* To configure later */
  int width = 900, height = 600;

  ctx->width = width;
  ctx->height = height;

  /* Windowing */
  glfwInit();

#if __APPLE__
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2); 
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
#endif

  ctx->window = glfwCreateWindow(ctx->width, ctx->height, "Glo", NULL, NULL);
  glfwMakeContextCurrent(ctx->window);

  /* OpenGL extensions */
  glewExperimental = GL_TRUE;
  glewInit();

  return ctx;
}

bool isContextClosed(DrawContext *ctx) {
  return glfwWindowShouldClose(ctx->window);
}

void tickDisplay(DrawContext *ctx) {
  glfwSwapBuffers(ctx->window);
  glfwPollEvents();
}

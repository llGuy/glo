#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "io.h"

/* Callback prototypes */
static void keyCallback(int,int,int,int);
static void mouseButtonCallback(int,int,int);
static void resizeCallback(unsigned int,unsigned int);
static void scrollCallback(float,float);
static void closeCallback();
static void cursorMoveCallback(float,float);

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

  ctx->dt = 0.0f;
  ctx->currentTime = glfwGetTime();

  return ctx;
}

bool isContextClosed(DrawContext *ctx) {
  return glfwWindowShouldClose(ctx->window);
}

void tickDisplay(DrawContext *ctx) {
  glfwSwapBuffers(ctx->window);
  glfwPollEvents();

  double newTime = glfwGetTime();
  double diff = newTime - ctx->currentTime;
  ctx->dt = (float)diff;
  ctx->currentTime = newTime;
}

GameCommands translateIO(DrawContext *ctx) {
  GameCommands commands = {};
  
  if (glfwGetKey(ctx->window, GLFW_KEY_W)) {
    commands.actions.moveUp = 1;
  }
  if (glfwGetKey(ctx->window, GLFW_KEY_A)) {
    commands.actions.moveLeft = 1;
  }
  if (glfwGetKey(ctx->window, GLFW_KEY_S)) {
    commands.actions.moveDown = 1;
  }
  if (glfwGetKey(ctx->window, GLFW_KEY_D)) {
    commands.actions.moveRight = 1;
  }
  if (glfwGetMouseButton(ctx->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    static double shootTime = 0.0f;

    double newShootTime = glfwGetTime();

    if (newShootTime - shootTime > 0.3) {
      shootTime = newShootTime;
      commands.actions.shoot = 1;
    }
  }

  double x, y;
  glfwGetCursorPos(ctx->window, &x, &y);
  y = (double)ctx->height - y;
  x -= (double)(ctx->width / 2);
  y -= (double)(ctx->height / 2);

  commands.newOrientation = (float)atan2(x,y);
  commands.dt = ctx->dt;

  return commands;
}

/* Callback definitions - todo if necessary */
static void keyCallback(int k, int s, int a, int m) {
  
}

static void mouseButtonCallback(int b, int a, int m) {
  
}

static void resizeCallback(unsigned int w, unsigned int h) {
  
}

static void scrollCallback(float x, float y) {
  
}

static void closeCallback() {
  
}

static void cursorMoveCallback(float x, float y) {
  
}

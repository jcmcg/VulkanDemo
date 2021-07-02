#pragma once

#include <windows.h>
#include <stdbool.h>

#define WIN_NAME "VulkanDemo"

typedef enum {
  WE_OK,
  WE_REG_WND_CLASS,
  WE_CREATE_WND
} WIN_ERROR;

typedef struct window_s {
  HINSTANCE hinstance;
  HWND hwnd;
  WORD width;
  WORD height;
  bool minimized;
  void (*on_create)();
  void (*on_destroy)();
  void (*on_size)();
  void (*on_paint)();
  void (*on_move)(int, int);
} window_t;

WIN_ERROR create_window(window_t *);
void cleanup_window(window_t *);

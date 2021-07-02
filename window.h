#pragma once

#include <windows.h>

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
  void (*on_create)();
  void (*on_destroy)();
} window_t;

WIN_ERROR create_window(window_t *);
void cleanup_window(window_t *);

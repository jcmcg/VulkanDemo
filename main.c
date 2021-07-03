#include "window.h"
#include "renderer.h"
#include "log.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
  image_t image = { 0 };
  IMAGE_ERROR ie = load_png("VulkanDemo.png", &image);
  if (ie) {
    LOG_DEBUG_ERROR("Could not load PNG: %s", IMAGE_ERRORS[ie]);
    return ie;
  }

  window_t window = { hInstance };
  window.width = 1024;
  window.height = 768;
  window.on_create = init_vulkan;
  window.on_destroy = cleanup_vulkan;
  window.on_size = resize;
  window.on_move = move;
  window.on_paint = render;
  vk_env.window = &window;
  vk_env.image = &image;

  int rc = E_FAIL;
  if (create_window(&window) == WE_OK) {
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else
        render();
    }
    rc = (int)msg.wParam;
  }

  cleanup_window(&window);
  destroy_image(&image);
  return rc;
}

#include "window.h"
#include "renderer.h"
#include "log.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
  int rc = E_FAIL;

  window_t window = { hInstance };
  window.width = 1024;
  window.height = 768;
  window.on_create = init_vulkan;
  window.on_destroy = cleanup_vulkan;
  vk_env.window = &window;

  if (create_window(&window) == WE_OK) {
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    rc = (int)msg.wParam;
  }
  cleanup_window(&window);

  LOG_DEBUG_INFO("Returning %d", rc);
  return rc;
}

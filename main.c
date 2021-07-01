#include "window.h"
#include "renderer.h"
#include "log.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
  int rc = E_FAIL;

  window_t window = { hInstance };
  window.width = 1024;
  window.height = 768;

  if (create_window(&window) == WE_OK) {
    MSG msg = { 0 };
    while (window.active) {
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    rc = (int)msg.wParam;
  }
  cleanup_window(&window);

  return rc;
}

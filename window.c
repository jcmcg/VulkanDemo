#include "window.h"

PCSTR WIN_ERRORS[] = {
  "OK",
  "Failed to register window class.",
  "Could not create main window."
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  window_t *window = uMsg == WM_CREATE
                   ? (window_t *)((CREATESTRUCT *)lParam)->lpCreateParams
                   : (window_t *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
  switch (uMsg) {
    case WM_CREATE:
      SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)window);
      window->hwnd = hWnd;
      window->on_create();
      break;
    case WM_DESTROY:
      SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
      window->on_destroy();
      PostQuitMessage(0);
      break;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT:
      ValidateRect(hWnd, NULL);
      window->on_paint();
      break;
    case WM_SIZE:
      window->width = LOWORD(lParam);
      window->height = HIWORD(lParam);
      window->on_size();
      break;
    case WM_KEYDOWN:
      switch (wParam) {
        case VK_ESCAPE:
          DestroyWindow(hWnd);
          break;
      }
      break;
    default:
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}

WIN_ERROR create_window(window_t *window) {
  WIN_ERROR we = WE_OK;

  RECT rect = { 0, 0, window->width, window->height };
  AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW);
  LONG width = rect.right - rect.left,
       height = rect.bottom - rect.top;

  WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = (WNDPROC)WndProc;
  wcex.hInstance = window->hinstance;
  wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.lpszClassName = WIN_NAME;
  wcex.hIconSm = LoadIcon(NULL, IDI_ASTERISK);
  if (!RegisterClassEx(&wcex))
    we = WE_REG_WND_CLASS;

  else if (!CreateWindowEx(
    WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW,
    WIN_NAME, WIN_NAME,
    WS_OVERLAPPEDWINDOW | WS_SYSMENU,
    CW_USEDEFAULT, CW_USEDEFAULT, width, height,
    NULL, NULL, window->hinstance, window
  ))
    we = WE_CREATE_WND;

  if (we)
    MessageBox(NULL, WIN_ERRORS[we], WIN_NAME, MB_OK | MB_ICONSTOP);
  else
    ShowWindow(window->hwnd, SW_SHOW);

  return we;
}

void cleanup_window(window_t *window) {
  UnregisterClass(WIN_NAME, window->hinstance);
}

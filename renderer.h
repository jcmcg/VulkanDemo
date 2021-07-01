#pragma once

#include <vulkan/vulkan.h>

#define ARRAY_COUNT(a) (sizeof (a) / sizeof (a[0]))

#ifdef _DEBUG
#define LOG_VK_ERROR(f, r) {               \
  log_vk_error(__FILE__, __LINE__, #f, r); \
  if (IsDebuggerPresent())                 \
    __debugbreak();                        \
}
#define VK_CALL(f) do {     \
  VkResult r = f;           \
  if (r) LOG_VK_ERROR(f, r) \
} while (0)
#define VK_CALL_EXT(f, i, ...) {                     \
  PFN_##f f = (PFN_##f)vkGetInstanceProcAddr(i, #f); \
  if (f) VK_CALL(f(i, __VA_ARGS__));                 \
  else LOG_VK_ERROR(vkGetInstanceProcAddr, 0)        \
}
#define VK_CALL_EXT_VOID(f, i, ...) {                \
  PFN_##f f = (PFN_##f)vkGetInstanceProcAddr(i, #f); \
  if (f) f(i, __VA_ARGS__);                          \
  else LOG_VK_ERROR(vkGetInstanceProcAddr, 0)        \
}
void log_vk_error(const char *, long, const char *, VkResult);
#else
#define LOG_DEBUG(...)
#define VK_CALL(f) f
#define VK_CALL_EXT(f, i, ...) {                     \
  PFN_##f f = (PFN_##f)vkGetInstanceProcAddr(i, #f); \
  if (f) VK_CALL(f(i, __VA_ARGS__))                  \
}
#define VK_CALL_EXT_VOID(f, i, ...) VK_CALL_EXT(f, i, __VA_ARGS__)
#endif // _DEBUG

typedef enum {
  VE_OK,
  VE_NO_INSTANCE_LAYERS,
  VE_INSTANCE_LAYER_UNAVAILABLE,
  VE_NO_INSTANCE_EXTENSIONS,
  VE_INSTANCE_EXTENSION_UNAVAILABLE
} VULKAN_ERROR;

typedef struct vk_env_s {
  VULKAN_ERROR error;
} vk_env_t;

vk_env_t vk_env;

void init_vulkan();
void cleanup_vulkan();

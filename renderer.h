#pragma once

#include <vulkan/vulkan.h>

#define APP_NAME "VulkanDemo"
#define APP_VERSION VK_MAKE_VERSION(1, 0, 0)
#define ENGINE_NAME "TestEngine"
#define ENGINE_VERSION VK_MAKE_VERSION(1, 0, 0)

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
#define VK_DEBUG_UTILS_MESSAGE_SEVERITY (           \
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   | \
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | \
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    | \
  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT   \
)
#define VK_DEBUG_UTILS_MESSAGE_TYPE (               \
  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     | \
  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | \
  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT    \
)
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

typedef struct cds_entry_s cds_entry_t;
typedef struct cds_entry_s {
  void (*destroy)();
  cds_entry_t *next;
} cds_entry_t;

typedef struct vk_env_s {
  VULKAN_ERROR error;
  cds_entry_t *cd_stack;
  VkInstance instance;
#ifdef _DEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vk_env_t;

vk_env_t vk_env;

void init_vulkan();
void cleanup_vulkan();

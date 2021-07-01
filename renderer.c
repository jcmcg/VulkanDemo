#include "renderer.h"
#include "window.h"
#include "log.h"
#include "heap.h"

vk_env_t vk_env = { VE_OK };

const char *VK_ERRORS[] = {
  "OK",
  "No validation layers available",
  "Required validation layer unavailable",
  "No instance extensions available",
  "Required instance extension unavailable"
};

#ifdef _DEBUG
const char *validation_layers[] = {
  "VK_LAYER_KHRONOS_validation"
};
const uint32_t validation_layer_count = ARRAY_COUNT(validation_layers);
#endif

const char *instance_extensions[] = {
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};
const uint32_t instance_extension_count = ARRAY_COUNT(instance_extensions);

#ifdef _DEBUG
void log_vk_error(const char *file, long line, const char *function, VkResult error) {
  log_console_error("VULKAN ERROR: '%s', line %ld, '%s' - VkResult: %d", file, line, function, error);
}

VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                             void *pUserData) {
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      log_console_error(pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      log_console_warning(pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    default:
      log_console_info(pCallbackData->pMessage);
      break;
  }
  return VK_FALSE;
}

VULKAN_ERROR enum_validation_layers() {
  LOG_DEBUG_INFO("enum_validation_layers()");
  uint32_t num_layers = 0, i, j;
  VK_CALL(vkEnumerateInstanceLayerProperties(&num_layers, NULL));
  if (!num_layers)
    return VE_NO_INSTANCE_LAYERS;
  VULKAN_ERROR ve = VE_OK;
  VkLayerProperties *layer_properties = halloc_type(VkLayerProperties, num_layers);
  VK_CALL(vkEnumerateInstanceLayerProperties(&num_layers, layer_properties));
  for (i = 0; i < validation_layer_count; i++) {
    for (j = 0;
         j < num_layers &&
         strcmp(validation_layers[i], layer_properties[j].layerName);
         j++);
    if (j == num_layers)
      ve = VE_INSTANCE_LAYER_UNAVAILABLE;
  }
  hfree(layer_properties);
  return ve;
}
#endif

VULKAN_ERROR enum_instance_extensions() {
  LOG_DEBUG_INFO("enum_instance_extensions()");
  uint32_t num_extensions = 0, i, j;
  VK_CALL(vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL));
  if (!num_extensions)
    return VE_NO_INSTANCE_EXTENSIONS;
  VULKAN_ERROR ve = VE_OK;
  VkExtensionProperties *extensions = halloc_type(VkExtensionProperties, num_extensions);
  VK_CALL(vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extensions));
  for (i = 0; i < instance_extension_count; i++) {
    for (j = 0;
         j < num_extensions &&
         strcmp(instance_extensions[i], extensions[j].extensionName);
         j++);
    if (j == num_extensions)
      ve = VE_INSTANCE_EXTENSION_UNAVAILABLE;
  }
  hfree(extensions);
  return ve;
}

void push_create(void (*create)(), void (*destroy)()) {
  cds_entry_t *cs_entry = halloc_type(cds_entry_t, 1);
  cs_entry->destroy = destroy;
  cs_entry->next = vk_env.cd_stack;
  vk_env.cd_stack = cs_entry;
  create();
}

void pop_destroy() {
  cds_entry_t *cs_entry = vk_env.cd_stack;
  cs_entry->destroy();
  vk_env.cd_stack = cs_entry->next;
  hfree(cs_entry);
}

void create_instance() {
  LOG_DEBUG_INFO("Begin create_instance()");

#ifdef _DEBUG
  // Pass debug_messenger_create_info to vkCreateInstance for temporary debug callback during instance creation
  VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
  debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY;
  debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE;
  debug_messenger_create_info.pfnUserCallback = debug_messenger_callback;
#endif
  VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
  app_info.pApplicationName = APP_NAME;
  app_info.applicationVersion = APP_VERSION;
  app_info.pEngineName = ENGINE_NAME;
  app_info.engineVersion = ENGINE_VERSION;
  app_info.apiVersion = VK_API_VERSION_1_2;
  VkInstanceCreateInfo instance_create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
#ifdef _DEBUG
  instance_create_info.pNext = &debug_messenger_create_info;
#endif
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledLayerCount = validation_layer_count;
  instance_create_info.ppEnabledLayerNames = validation_layers;
  instance_create_info.enabledExtensionCount = instance_extension_count;
  instance_create_info.ppEnabledExtensionNames = instance_extensions;
  VK_CALL(vkCreateInstance(&instance_create_info, NULL, &vk_env.instance));
  VK_CALL_EXT(vkCreateDebugUtilsMessengerEXT, vk_env.instance, &debug_messenger_create_info, NULL, &vk_env.debug_messenger);

  LOG_DEBUG_INFO("End create_instance()");
}

void destroy_instance() {
  LOG_DEBUG_INFO("Begin destroy_instance()");

#ifdef _DEBUG
  VK_CALL_EXT_VOID(vkDestroyDebugUtilsMessengerEXT, vk_env.instance, vk_env.debug_messenger, NULL);
#endif
  vkDestroyInstance(vk_env.instance, NULL);

  LOG_DEBUG_INFO("End destroy_instance()");
}

void create_surface() {
  VkWin32SurfaceCreateInfoKHR surface_create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
  surface_create_info.hinstance = vk_env.window->hinstance;
  surface_create_info.hwnd = vk_env.window->hwnd;
  VK_CALL(vkCreateWin32SurfaceKHR(vk_env.instance, &surface_create_info, NULL, &vk_env.surface));
  LOG_DEBUG_INFO("Created Vulkan Win32KHR surface");
}

void destroy_surface() {
  vkDestroySurfaceKHR(vk_env.instance, vk_env.surface, NULL);
  LOG_DEBUG_INFO("Destroyed Vulkan Win32KHR surface");
}

void init_vulkan() {
  LOG_DEBUG_INFO("Begin init_vulkan()");

  if (
#ifdef _DEBUG
    (vk_env.error = enum_validation_layers()) ||
#endif
    (vk_env.error = enum_instance_extensions())
  ) {
    LOG_DEBUG_ERROR(VK_ERRORS[vk_env.error]);
    return;
  }

  push_create(create_instance, destroy_instance);
  push_create(create_surface, destroy_surface);

  LOG_DEBUG_INFO("End init_vulkan()");
}

void cleanup_vulkan() {
  LOG_DEBUG_INFO("Begin cleanup_vulkan()");

  while (vk_env.cd_stack)
    pop_destroy();

  LOG_DEBUG_INFO("End cleanup_vulkan()");
}

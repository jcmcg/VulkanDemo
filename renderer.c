#include "renderer.h"
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

  LOG_DEBUG_INFO("End init_vulkan()");
}

void cleanup_vulkan() {
  LOG_DEBUG_INFO("Begin cleanup_vulkan()");

  LOG_DEBUG_INFO("End cleanup_vulkan()");
}

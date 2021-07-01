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

const char *device_extensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const uint32_t device_extension_count = ARRAY_COUNT(device_extensions);

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

VULKAN_ERROR select_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkSurfaceFormatKHR *surface_format) {
  uint32_t num_formats, i = 0;
  VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, NULL));
  if (!num_formats)
    return VE_NO_SURFACE_FORMATS;

  VULKAN_ERROR ve = VE_OK;
  VkSurfaceFormatKHR *surface_formats = halloc_type(VkSurfaceFormatKHR, num_formats);
  VK_CALL(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, surface_formats));

  if (num_formats == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
    // Device supports any format
    surface_formats[0].format = VK_FORMAT_B8G8R8A8_UNORM;
    surface_formats[0].colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  }
  else
    for (i = 0;
         i < num_formats &&
         !(surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
           surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) &&
         surface_formats[i].colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
         i++);
  if (i < num_formats) {
    *surface_format = surface_formats[i];
    LOG_DEBUG_INFO("Selected surface format: %d", surface_format->format);
  }
  else
    ve = VE_NO_SUITABLE_SURFACE_FORMAT;

  hfree(surface_formats);
  return ve;
}

VkBool32 set_num_buffers(VkPhysicalDevice physical_device, VkSurfaceKHR surface, GPU *gpu, uint32_t num_requested) {
  LOG_DEBUG_INFO("%s buffering requested", num_requested == 2 ? "Double" : "Triple");
  gpu->num_buffers = num_requested;
  if (!(num_requested == 2 || num_requested == 3))
    return VK_FALSE;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities));
  CLAMP(gpu->num_buffers, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
  return gpu->num_buffers == num_requested;
}

VkBool32 set_present_mode(VkPresentModeKHR *present_modes, uint32_t num_modes,
                          VkPresentModeKHR req_present_mode, VkPresentModeKHR *present_mode) {
  uint32_t i;
  for (i = 0;
       i < num_modes &&
       present_modes[i] != req_present_mode;
       i++);
  if (i < num_modes) {
    *present_mode = req_present_mode;
    LOG_DEBUG_INFO("Selected presentation mode: %d", req_present_mode);
    return VK_TRUE;
  }
  return VK_FALSE;
}

VULKAN_ERROR select_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkPresentModeKHR *present_mode) {
  uint32_t num_modes;
  VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_modes, NULL));
  if (!num_modes)
    return VE_NO_PRESENT_MODES;

  VULKAN_ERROR ve = VE_OK;
  VkPresentModeKHR *present_modes = halloc_type(VkPresentModeKHR, num_modes);
  VK_CALL(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_modes, present_modes));
  // Ideal: VK_PRESENT_MODE_MAILBOX_KHR
  // Fallback: VK_PRESENT_MODE_FIFO_KHR (required to be supported)
  if (!(set_present_mode(present_modes, num_modes, VK_PRESENT_MODE_MAILBOX_KHR, present_mode) ||
        set_present_mode(present_modes, num_modes, VK_PRESENT_MODE_FIFO_KHR, present_mode)))
    ve = VE_NO_SUITABLE_PRESENT_MODE;
  hfree(present_modes);

  return ve;
}

VULKAN_ERROR select_physical_device(uint32_t num_buffers) {
  LOG_DEBUG_INFO("Begin select_physical_device()");

  uint32_t num_gpus = 0, num_extensions, num_queue_families, i, j;
  VK_CALL(vkEnumeratePhysicalDevices(vk_env.instance, &num_gpus, NULL));
  if (!num_gpus)
    return VE_NO_PHYSICAL_DEVICES;
  VkPhysicalDevice *physical_devices = halloc_type(VkPhysicalDevice, num_gpus);
  VK_CALL(vkEnumeratePhysicalDevices(vk_env.instance, &num_gpus, physical_devices));
  GPU *gpus = halloc_type(GPU, num_gpus);

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  GPU_SUPPORT best = GPU_SUPPORT_NONE;
  int selected = -1;
  VULKAN_ERROR ve = VE_NO_SUPPORTED_PHYSICAL_DEVICE;
  for (i = 0; i < num_gpus; i++) {
    gpus[i].device = physical_devices[i];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &num_queue_families, NULL);
    if (!num_queue_families)
      continue;
    gpus[i].graphics_qfi = UINT32_MAX;
    gpus[i].present_qfi = UINT32_MAX;
    VkQueueFamilyProperties *queue_families = halloc_type(VkQueueFamilyProperties, num_queue_families);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &num_queue_families, queue_families);
    for (j = 0; j < num_queue_families; j++) {
      if (gpus[i].graphics_qfi == UINT32_MAX &&
          queue_families[j].queueCount &&
          (queue_families[j].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)))
        gpus[i].graphics_qfi = j;
      if (gpus[i].present_qfi == UINT32_MAX) {
        VkBool32 supported = VK_FALSE;
        VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, vk_env.surface, &supported));
        if (supported)
          gpus[i].present_qfi = j;
      }
    }
    hfree(queue_families);
    if (gpus[i].graphics_qfi == UINT32_MAX || gpus[i].present_qfi == UINT32_MAX)
      continue;

    gpus[i].support = GPU_SUPPORT_NONE;
    VK_CALL(vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &num_extensions, NULL));
    if (!num_extensions)
      continue;
    VkExtensionProperties *extensions = halloc_type(VkExtensionProperties, num_extensions);
    VK_CALL(vkEnumerateDeviceExtensionProperties(physical_devices[i], NULL, &num_extensions, extensions));
    for (j = 0; j < num_extensions; j++) {
      if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, extensions[j].extensionName))
        gpus[i].support |= GPU_SUPPORT_RASTERIZATION;
      else if (!strcmp(VK_NV_RAY_TRACING_EXTENSION_NAME, extensions[j].extensionName))
        gpus[i].support |= GPU_SUPPORT_RAYTRACING;
    }
    hfree(extensions);

    if (select_surface_format(physical_devices[i], vk_env.surface, &gpus[i].surface_format) ||
        !set_num_buffers(physical_devices[i], vk_env.surface, &gpus[i], num_buffers) ||
        select_present_mode(physical_devices[i], vk_env.surface, &gpus[i].present_mode))
      continue;

    vkGetPhysicalDeviceFeatures(physical_devices[i], &features);
    if (features.textureCompressionBC)
      gpus[i].support |= GPU_SUPPORT_TEXTURE_COMPRESSION;
    if (features.samplerAnisotropy)
      gpus[i].support |= GPU_SUPPORT_ANISTROPIC_FILTERING;

    vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
    gpus[i].name = properties.deviceName;

    if (gpus[i].support > best) {
      best = gpus[i].support;
      selected = i;
    }
  }

  if (selected >= 0) {
    GPU *gpu = &gpus[selected];
    vkGetPhysicalDeviceMemoryProperties(gpu->device, &gpu->memory_properties);
    memcpy(&vk_env.gpu, gpu, sizeof (GPU));
    vk_env.gpu.name = _strdup(gpu->name);
    if (gpu->graphics_qfi != gpu->present_qfi)
      vk_env.distinct_qfi = true;
    LOG_DEBUG_INFO("Selected physical device: %s", gpu->name);
    ve = VE_OK;
  }

  hfree(gpus);
  hfree(physical_devices);

  LOG_DEBUG_INFO("End select_physical_device()");
  return ve;
}

void create_logical_device() {
  LOG_DEBUG_INFO("Begin create_logical_device()");

  // Number of queues will be 1 or 2 depending on whether or not graphics_qfi == present_qfi
  uint32_t num_queues = 1 + vk_env.distinct_qfi;
  const float queue_priorities[1] = { 1.0f };
  VkDeviceQueueCreateInfo *queue_create_info = halloc_clear_type(VkDeviceQueueCreateInfo, num_queues);
  for (uint32_t i = 0; i < num_queues; i++) {
    queue_create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[i].queueFamilyIndex = i ? vk_env.gpu.present_qfi : vk_env.gpu.graphics_qfi;
    queue_create_info[i].queueCount = 1;
    queue_create_info[i].pQueuePriorities = queue_priorities;
  }

  VkPhysicalDeviceFeatures device_features = { 0 };
  device_features.textureCompressionBC = VK_TRUE;
  device_features.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
  device_info.queueCreateInfoCount = num_queues;
  device_info.pQueueCreateInfos = queue_create_info;
  device_info.enabledExtensionCount = device_extension_count;
  device_info.ppEnabledExtensionNames = device_extensions;
  device_info.pEnabledFeatures = &device_features;
  VK_CALL(vkCreateDevice(vk_env.gpu.device, &device_info, NULL, &vk_env.device));

  vkGetDeviceQueue(vk_env.device, vk_env.gpu.graphics_qfi, 0, &vk_env.graphics_queue);
  if (vk_env.distinct_qfi)
    vkGetDeviceQueue(vk_env.device, vk_env.gpu.present_qfi, 0, &vk_env.present_queue);
  else
    vk_env.present_queue = vk_env.graphics_queue;

  hfree(queue_create_info);

  LOG_DEBUG_INFO("End create_logical_device()");
}

void destroy_logical_device() {
  LOG_DEBUG_INFO("Begin destroy_logical_device()");
  vkDestroyDevice(vk_env.device, NULL);
  LOG_DEBUG_INFO("End destroy_logical_device()");
}

void create_command_pool() {
  VkCommandPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  create_info.queueFamilyIndex = vk_env.gpu.graphics_qfi;
  VK_CALL(vkCreateCommandPool(vk_env.device, &create_info, NULL, &vk_env.command_pool));
  LOG_DEBUG_INFO("Created command pool");
}

void destroy_command_pool() {
  vkDestroyCommandPool(vk_env.device, vk_env.command_pool, NULL);
  LOG_DEBUG_INFO("Destroyed command pool");
}

void create_command_buffers() {
  VkCommandBufferAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocate_info.commandPool = vk_env.command_pool;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocate_info.commandBufferCount = vk_env.gpu.num_buffers;
  vk_env.command_buffers = halloc_type(VkCommandBuffer, vk_env.gpu.num_buffers);
  VK_CALL(vkAllocateCommandBuffers(vk_env.device, &allocate_info, vk_env.command_buffers));
  LOG_DEBUG_INFO("Created %d command buffers", vk_env.gpu.num_buffers);
}

void destroy_command_buffers() {
  vkFreeCommandBuffers(vk_env.device, vk_env.command_pool, vk_env.gpu.num_buffers, vk_env.command_buffers);
  hfree(vk_env.command_buffers);
  LOG_DEBUG_INFO("Destroyed %d command buffers", vk_env.gpu.num_buffers);
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

  if ((vk_env.error = select_physical_device(NUM_BUFFERS))) {
    LOG_DEBUG_ERROR(VK_ERRORS[vk_env.error]);
    return;
  }

  push_create(create_logical_device, destroy_logical_device);
  push_create(create_command_pool, destroy_command_pool);
  push_create(create_command_buffers, destroy_command_buffers);

  LOG_DEBUG_INFO("End init_vulkan()");
}

void cleanup_vulkan() {
  LOG_DEBUG_INFO("Begin cleanup_vulkan()");

  while (vk_env.cd_stack)
    pop_destroy();
  if (vk_env.gpu.name)
    hfree(vk_env.gpu.name);

  LOG_DEBUG_INFO("End cleanup_vulkan()");
}

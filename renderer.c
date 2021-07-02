#include <stdio.h>
#include "renderer.h"
#include "window.h"
#include "log.h"
#include "heap.h"

vk_env_t vk_env = { VE_OK };

const vertex_t rect[] = {
  // position             // colour
{ {  0.8f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
{ { -0.8f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
{ {  0.8f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },

{ { -0.8f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
{ { -0.8f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
{ {  0.8f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

float mvp[] = {
  1.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 1.0f
};

const char *VK_ERRORS[] = {
  "OK",
  "No validation layers available",
  "Required validation layer unavailable",
  "No instance extensions available",
  "Required instance extension unavailable",
  "No physical devices available",
  "No physical device with rasterization support available",
  "Could not open shader file",
  "Could not read shader file"
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
  if (create)
    create();
}

void pop_destroy() {
  cds_entry_t *cs_entry = vk_env.cd_stack;
  if (cs_entry->destroy)
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
          FLAGGED(queue_families[j].queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
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

void create_sync_objects() {
  VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
  // Allow maximum of (num_buffers - 1) concurrent frames
  vk_env.frame_lag = vk_env.gpu.num_buffers - 1;
  vk_env.fences = halloc_type(VkFence, vk_env.frame_lag);
  vk_env.image_acquired_semaphores = halloc_type(VkSemaphore, vk_env.frame_lag);
  vk_env.draw_complete_semaphores = halloc_type(VkSemaphore, vk_env.frame_lag);
  if (vk_env.distinct_qfi)
    vk_env.image_ownership_semaphores = halloc_type(VkSemaphore, vk_env.frame_lag);
  uint32_t i;
  for (i = 0; i < vk_env.frame_lag; i++) {
    VK_CALL(vkCreateFence(vk_env.device, &fence_info, NULL, &vk_env.fences[i]));
    VK_CALL(vkCreateSemaphore(vk_env.device, &semaphore_info, NULL, &vk_env.image_acquired_semaphores[i]));
    VK_CALL(vkCreateSemaphore(vk_env.device, &semaphore_info, NULL, &vk_env.draw_complete_semaphores[i]));
    if (vk_env.distinct_qfi)
      VK_CALL(vkCreateSemaphore(vk_env.device, &semaphore_info, NULL, &vk_env.image_ownership_semaphores[i]));
  }
  LOG_DEBUG_INFO("Created %d fences and %d semaphores", i, i * (2 + vk_env.distinct_qfi));
}

void destroy_sync_objects() {
  uint32_t i;
  for (i = 0; i < vk_env.frame_lag; i++) {
    vkWaitForFences(vk_env.device, 1, &vk_env.fences[i], VK_TRUE, UINT64_MAX);
    vkDestroyFence(vk_env.device, vk_env.fences[i], NULL);
    vkDestroySemaphore(vk_env.device, vk_env.image_acquired_semaphores[i], NULL);
    vkDestroySemaphore(vk_env.device, vk_env.draw_complete_semaphores[i], NULL);
    if (vk_env.distinct_qfi)
      vkDestroySemaphore(vk_env.device, vk_env.image_ownership_semaphores[i], NULL);
  }
  hfree(vk_env.fences);
  hfree(vk_env.image_acquired_semaphores);
  hfree(vk_env.draw_complete_semaphores);
  if (vk_env.distinct_qfi)
    hfree(vk_env.image_ownership_semaphores);
  LOG_DEBUG_INFO("Destroyed %d fences and %d semaphores", i, i * (2 + vk_env.distinct_qfi));
}

VkResult create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask, VkImageView *image_view) {
  VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
  create_info.image = image;
  create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  create_info.format = format;
  create_info.subresourceRange.aspectMask = aspect_mask;
  create_info.subresourceRange.levelCount = 1;
  create_info.subresourceRange.layerCount = 1;
  return vkCreateImageView(device, &create_info, NULL, image_view);
}

void create_render_pass() {
  LOG_DEBUG_INFO("Begin create_render_pass()");

  VkAttachmentDescription attachments[1] = { 0 }; // colour attachment
  VkAttachmentDescription *colour_desc = &attachments[0];
  colour_desc->format = vk_env.gpu.surface_format.format;
  colour_desc->samples = VK_SAMPLE_COUNT_1_BIT;
  colour_desc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_desc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_desc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_desc->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
  // The image will automatically be transitioned from
  // VK_IMAGE_LAYOUT_UNDEFINED to VK_COLOR_ATTACHMENT_OPTIMAL for rendering,
  // then out to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR at the end.
  const VkAttachmentReference colour_attachment = {
    0, // attachment
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
  };

  VkSubpassDescription subpass = { 0 };
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colour_attachment;
  VkRenderPassCreateInfo create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
  create_info.attachmentCount = 1;
  create_info.pAttachments = attachments;
  create_info.subpassCount = 1;
  create_info.pSubpasses = &subpass;
  VK_CALL(vkCreateRenderPass(vk_env.device, &create_info, NULL, &vk_env.render_pass));

  LOG_DEBUG_INFO("End create_render_pass()");
}

void destroy_render_pass() {
  vkDestroyRenderPass(vk_env.device, vk_env.render_pass, NULL);
  LOG_DEBUG_INFO("Destroyed render pass");
}

VULKAN_ERROR create_shader_module(const char *stage) {
  char path[64];
  snprintf(path, 64, "shaders\\spv\\%s.%s.spv", SHADER_NAME, stage);
  LOG_DEBUG_INFO("Loading shader '%s'...", path);

  FILE *fp;
  if (fopen_s(&fp, path, "rb"))
    return VE_SHADER_FILE_OPEN;
  long length = -1;
  if (!fseek(fp, 0, SEEK_END)) {
    length = ftell(fp);
    rewind(fp);
  }
  VULKAN_ERROR ve = VE_OK;
  uint32_t *code = NULL;
  if (length <= 0 || (length % 4))
    ve = VE_SHADER_FILE_READ;
  else {
    code = (uint32_t *)halloc(length);
    if (fread(code, length, 1, fp) != 1)
      ve = VE_SHADER_FILE_READ;
  }
  fclose(fp);

  if (code) {
    if (!ve) {
      VkShaderModuleCreateInfo shader_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
      shader_info.codeSize = (size_t)length;
      shader_info.pCode = code;
      VK_CALL(vkCreateShaderModule(
        vk_env.device, &shader_info, NULL,
        *stage == 'v' ? &vk_env.vertex_shader : &vk_env.fragment_shader
      ));
      LOG_DEBUG_INFO("Created shader module: %s.%s", SHADER_NAME, stage);
    }
    hfree(code);
  }

  return ve;
}

void destroy_shader_module(const char *stage) {
  vkDestroyShaderModule(vk_env.device, *stage == 'v' ? vk_env.vertex_shader : vk_env.fragment_shader, NULL);
  LOG_DEBUG_INFO("Destroyed shader module: %s.%s", SHADER_NAME, stage);
}

void create_shader_modules() {
  (vk_env.error = create_shader_module("vert")) ||
  (vk_env.error = create_shader_module("frag"));
}

void destroy_shader_modules() {
  destroy_shader_module("vert");
  destroy_shader_module("frag");
}

void alloc_device_memory(VkMemoryRequirements requirements, VkFlags flags, VkDeviceMemory *device_memory) {
  VkMemoryAllocateInfo mem_alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
  mem_alloc_info.allocationSize = requirements.size;
  uint32_t index;
  for (
    index = 0;
    index < VK_MAX_MEMORY_TYPES &&
    !(FLAGGED(requirements.memoryTypeBits, 1 << index) &&
      FLAGGED(vk_env.gpu.memory_properties.memoryTypes[index].propertyFlags, flags));
    index++
    );
  mem_alloc_info.memoryTypeIndex = index;
  VK_CALL(vkAllocateMemory(vk_env.device, &mem_alloc_info, NULL, device_memory));
  LOG_DEBUG_INFO("Allocated %llu bytes of device memory", requirements.size);
}

void create_vertex_buffer() {
  LOG_DEBUG_INFO("Begin create_vertex_buffer()");

  VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  buffer_info.size = sizeof rect;
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  VK_CALL(vkCreateBuffer(vk_env.device, &buffer_info, NULL, &vk_env.rect_vb.buffer));
  LOG_DEBUG_INFO("Created vertex buffer");

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(vk_env.device, vk_env.rect_vb.buffer, &memory_requirements);
  alloc_device_memory(
    memory_requirements,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    &vk_env.rect_vb.device_memory
  );
  VK_CALL(vkBindBufferMemory(vk_env.device, vk_env.rect_vb.buffer, vk_env.rect_vb.device_memory, 0));
  void *data;
  VK_CALL(vkMapMemory(vk_env.device, vk_env.rect_vb.device_memory, 0, sizeof rect, 0, &data));
  memcpy(data, rect, sizeof rect);
  vkUnmapMemory(vk_env.device, vk_env.rect_vb.device_memory);
  LOG_DEBUG_INFO("Loaded vertex buffer into device memory");

  LOG_DEBUG_INFO("End create_vertex_buffer()");
}

void destroy_vertex_buffer() {
  LOG_DEBUG_INFO("Begin destroy_vertex_buffer()");

  vkFreeMemory(vk_env.device, vk_env.rect_vb.device_memory, NULL);
  LOG_DEBUG_INFO("Freed vertex buffer device memory");
  vkDestroyBuffer(vk_env.device, vk_env.rect_vb.buffer, NULL);
  LOG_DEBUG_INFO("Destroyed vertex buffer");

  LOG_DEBUG_INFO("End destroy_vertex_buffer()");
}

void create_uniform_buffer() {
  LOG_DEBUG_INFO("Begin create_uniform_buffer()");

  VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
  buffer_info.size = sizeof mvp;
  buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  VK_CALL(vkCreateBuffer(vk_env.device, &buffer_info, NULL, &vk_env.mvp_ub.buffer));
  LOG_DEBUG_INFO("Created uniform buffer");

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(vk_env.device, vk_env.mvp_ub.buffer, &memory_requirements);
  alloc_device_memory(
    memory_requirements,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &vk_env.mvp_ub.device_memory
  );
  VK_CALL(vkBindBufferMemory(vk_env.device, vk_env.mvp_ub.buffer, vk_env.mvp_ub.device_memory, 0));
  VK_CALL(vkMapMemory(vk_env.device, vk_env.mvp_ub.device_memory, 0, sizeof mvp, 0, &vk_env.mvp_ub.mem_ptr));
  LOG_DEBUG_INFO("Mapped uniform buffer to device memory");

  LOG_DEBUG_INFO("End create_uniform_buffer()");
}

void destroy_uniform_buffer() {
  LOG_DEBUG_INFO("Begin destroy_uniform_buffer()");

  vkUnmapMemory(vk_env.device, vk_env.mvp_ub.device_memory);
  LOG_DEBUG_INFO("Unmapped uniform buffer pointer");
  vkFreeMemory(vk_env.device, vk_env.mvp_ub.device_memory, NULL);
  LOG_DEBUG_INFO("Freed uniform buffer device memory");
  vkDestroyBuffer(vk_env.device, vk_env.mvp_ub.buffer, NULL);
  LOG_DEBUG_INFO("Destroyed uniform buffer");

  LOG_DEBUG_INFO("End destroy_uniform_buffer()");
}

void create_layouts() {
  // Descriptor set layout
  VkDescriptorSetLayoutBinding ubo_binding = { 0 };
  ubo_binding.binding = 0;
  ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_binding.descriptorCount = 1;
  ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  VkDescriptorSetLayoutBinding bindings[] = { ubo_binding };

  VkDescriptorSetLayoutCreateInfo descriptor_set_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
  descriptor_set_info.bindingCount = ARRAY_COUNT(bindings);
  descriptor_set_info.pBindings = bindings;
  VK_CALL(vkCreateDescriptorSetLayout(vk_env.device, &descriptor_set_info, NULL, &vk_env.descriptor_set_layout));
  LOG_DEBUG_INFO("Created descriptor set layout");

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &vk_env.descriptor_set_layout;
  VK_CALL(vkCreatePipelineLayout(vk_env.device, &pipeline_layout_info, NULL, &vk_env.pipeline_layout));
  LOG_DEBUG_INFO("Created pipeline layout");
}

void destroy_layouts() {
  vkDestroyPipelineLayout(vk_env.device, vk_env.pipeline_layout, NULL);
  LOG_DEBUG_INFO("Destroyed pipeline layout");
}

void create_descriptor_pool() {
  const VkDescriptorPoolSize pool_sizes[] = {
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
  };
  VkDescriptorPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
  create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  create_info.maxSets = vk_env.gpu.num_buffers;
  create_info.poolSizeCount = ARRAY_COUNT(pool_sizes);
  create_info.pPoolSizes = pool_sizes;
  VK_CALL(vkCreateDescriptorPool(vk_env.device, &create_info, NULL, &vk_env.descriptor_pool));

  LOG_DEBUG_INFO("Created descriptor pool");
}

void destroy_descriptor_pool() {
  vkDestroyDescriptorPool(vk_env.device, vk_env.descriptor_pool, NULL);
  LOG_DEBUG_INFO("Destroyed descriptor pool");
}

void alloc_descriptor_sets() {
  VkDescriptorSetAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
  allocate_info.descriptorPool = vk_env.descriptor_pool;
  allocate_info.descriptorSetCount = 1;
  allocate_info.pSetLayouts = &vk_env.descriptor_set_layout;
  VkDescriptorBufferInfo buffer_info = { 0 };
  buffer_info.buffer = vk_env.mvp_ub.buffer;
  buffer_info.range = sizeof mvp;
  VkWriteDescriptorSet writes[] = {
    { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }
  };
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].pBufferInfo = &buffer_info;
  vk_env.descriptor_sets = halloc_type(VkDescriptorSet, vk_env.gpu.num_buffers);
  for (uint32_t i = 0; i < vk_env.gpu.num_buffers; i++) {
    VK_CALL(vkAllocateDescriptorSets(vk_env.device, &allocate_info, &vk_env.descriptor_sets[i]));
    writes[0].dstSet = vk_env.descriptor_sets[i];
    vkUpdateDescriptorSets(vk_env.device, ARRAY_COUNT(writes), writes, 0, NULL);
  }
  LOG_DEBUG_INFO("Allocated %d uniform buffer descriptor sets", vk_env.gpu.num_buffers);
}

void free_descriptor_sets() {
  VK_CALL(vkFreeDescriptorSets(vk_env.device, vk_env.descriptor_pool, vk_env.gpu.num_buffers, vk_env.descriptor_sets));
  hfree(vk_env.descriptor_sets);
  LOG_DEBUG_INFO("Freed %d uniform buffer descriptor sets", vk_env.gpu.num_buffers);
}

void create_pipeline_cache() {
  VkPipelineCacheCreateInfo pipeline_cache_create_info = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
  VK_CALL(vkCreatePipelineCache(vk_env.device, &pipeline_cache_create_info, NULL, &vk_env.pipeline_cache));
  LOG_DEBUG_INFO("Created pipeline cache");
}

void destroy_pipeline_cache() {
  vkDestroyPipelineCache(vk_env.device, vk_env.pipeline_cache, NULL);
  LOG_DEBUG_INFO("Destroyed pipeline cache");
}

void create_pipeline() {
  LOG_DEBUG_INFO("Begin create_pipeline()");

  // Shader stages
  VkPipelineShaderStageCreateInfo shader_stages[] = {
    { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
    { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }
  };
  shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader_stages[0].module = vk_env.vertex_shader;
  shader_stages[0].pName = SHADER_ENTRY_POINT_NAME;
  shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader_stages[1].module = vk_env.fragment_shader;
  shader_stages[1].pName = SHADER_ENTRY_POINT_NAME;

  // Vertex input state
  VkVertexInputBindingDescription binding_description = { 0 };
  binding_description.binding = 0;
  binding_description.stride = sizeof (vertex_t);
  binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  VkVertexInputAttributeDescription position_attribute = { 0 };
  position_attribute.location = 0;
  position_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_attribute.offset = offsetof(vertex_t, position);
  VkVertexInputAttributeDescription colour_attribute = { 0 };
  colour_attribute.location = 1;
  colour_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
  colour_attribute.offset = offsetof(vertex_t, colour);
  VkVertexInputAttributeDescription attribute_descriptions[] = { position_attribute, colour_attribute };
  VkPipelineVertexInputStateCreateInfo vertex_input_state = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  vertex_input_state.vertexBindingDescriptionCount = 1;
  vertex_input_state.pVertexBindingDescriptions = &binding_description;
  vertex_input_state.vertexAttributeDescriptionCount = ARRAY_COUNT(attribute_descriptions);
  vertex_input_state.pVertexAttributeDescriptions = attribute_descriptions;

  // Input assembly state
  VkPipelineInputAssemblyStateCreateInfo input_assembly_state = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  // Viewport state (dynamic)
  VkPipelineViewportStateCreateInfo viewport_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  // Rasterization state
  VkPipelineRasterizationStateCreateInfo rasterization_state = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization_state.depthBiasEnable = VK_FALSE;
  rasterization_state.lineWidth = 1.0f;

  // Multisample state
  VkPipelineMultisampleStateCreateInfo multisample_state = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // vkcontext.sampleCount

  // Colour blending
  VkPipelineColorBlendAttachmentState colour_blend_attachment_state = { 0 };
  colour_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                 VK_COLOR_COMPONENT_G_BIT |
                                                 VK_COLOR_COMPONENT_B_BIT |
                                                 VK_COLOR_COMPONENT_A_BIT;
  VkPipelineColorBlendStateCreateInfo colour_blend_state = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  colour_blend_state.attachmentCount = 1;
  colour_blend_state.pAttachments = &colour_blend_attachment_state;

  // Dynamic states
  const VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo dynamic_state = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
  dynamic_state.dynamicStateCount = ARRAY_COUNT(dynamic_states);
  dynamic_state.pDynamicStates = dynamic_states;

  VkGraphicsPipelineCreateInfo create_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  create_info.stageCount = ARRAY_COUNT(shader_stages);
  create_info.pStages = shader_stages;
  create_info.pVertexInputState = &vertex_input_state;
  create_info.pInputAssemblyState = &input_assembly_state;
  create_info.pViewportState = &viewport_state;
  create_info.pRasterizationState = &rasterization_state;
  create_info.pMultisampleState = &multisample_state;
  create_info.pColorBlendState = &colour_blend_state;
  create_info.pDynamicState = &dynamic_state;
  create_info.layout = vk_env.pipeline_layout;
  create_info.renderPass = vk_env.render_pass;
  VK_CALL(vkCreateGraphicsPipelines(vk_env.device, vk_env.pipeline_cache, 1, &create_info, NULL, &vk_env.pipeline));

  LOG_DEBUG_INFO("End create_pipeline()");
}

void destroy_pipeline() {
  vkDestroyPipeline(vk_env.device, vk_env.pipeline, NULL);
  LOG_DEBUG_INFO("Destroyed pipeline");
}

void destroy_swapchain(VkSwapchainKHR swapchain) {
  VK_CALL(vkDeviceWaitIdle(vk_env.device));
  for (uint32_t i = 0; i < vk_env.gpu.num_buffers; i++) {
    vkDestroyFramebuffer(vk_env.device, vk_env.framebuffers[i], NULL);
    vkDestroyImageView(vk_env.device, vk_env.swapchain_views[i], NULL);
  }
  hfree(vk_env.framebuffers);
  hfree(vk_env.swapchain_views);
  hfree(vk_env.swapchain_images);
  LOG_DEBUG_INFO("Destroyed %d swapchain images, views and framebuffers", vk_env.gpu.num_buffers);
  vkDestroySwapchainKHR(vk_env.device, swapchain, NULL);
}

void create_swapchain() {
  LOG_DEBUG_INFO("Begin create_swapchain()");

  VkSurfaceCapabilitiesKHR surface_capabilities;
  VK_CALL(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_env.gpu.device, vk_env.surface, &surface_capabilities));
  VkExtent2D swapchain_extent;
  if (surface_capabilities.currentExtent.width == UINT32_MAX) {
    swapchain_extent.width = vk_env.window->width;
    swapchain_extent.height = vk_env.window->height;
    // Clamp to values allowed by the GPU
    CLAMP(swapchain_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    CLAMP(swapchain_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
  }
  else {
    swapchain_extent = surface_capabilities.currentExtent;
    vk_env.window->width = swapchain_extent.width;
    vk_env.window->height = swapchain_extent.height;
  }

  if (swapchain_extent.width && swapchain_extent.height) {
    LOG_DEBUG_INFO("Swapchain extent: (%d, %d)", swapchain_extent.width, swapchain_extent.height);
    vk_env.window->minimized = false;
  }
  else {
    // This happens if there's a VK_ERROR_OUT_OF_DATE_KHR error during a render cycle
    LOG_DEBUG_WARNING("Window minimized - swapchain not created");
    vk_env.window->minimized = true;
    return;
  }

  VkSwapchainKHR old_swapchain = vk_env.swapchain;
  VkSwapchainCreateInfoKHR swapchain_create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
  swapchain_create_info.surface = vk_env.surface;
  swapchain_create_info.minImageCount = vk_env.gpu.num_buffers;
  swapchain_create_info.imageFormat = vk_env.gpu.surface_format.format;
  swapchain_create_info.imageColorSpace = vk_env.gpu.surface_format.colorSpace;
  swapchain_create_info.imageExtent = swapchain_extent;
  swapchain_create_info.imageArrayLayers = 1;
  // swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  // ERROR Validation Error : [VUID - vkCmdClearColorImage - image - 00002] Object 0 :
  // handle = 0xd76249000000000c, type = VK_OBJECT_TYPE_IMAGE; | MessageID = 0x636f8691 |
  // vkCmdClearColorImage() pRanges[0] called with image VkImage 0xd76249000000000c[]
  // which was created without VK_IMAGE_USAGE_TRANSFER_DST_BIT.The Vulkan spec states :
  // image must have been created with VK_IMAGE_USAGE_TRANSFER_DST_BIT usage flag
  // (https ://vulkan.lunarg.com/doc/view/1.2.154.1/windows/1.2-extensions/vkspec.html#VUID-vkCmdClearColorImage-image-00002)
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  uint32_t queue_family_indexes[] = { vk_env.gpu.graphics_qfi, vk_env.gpu.present_qfi };
  if (vk_env.distinct_qfi) {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_family_indexes;
  }
  else {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices = &vk_env.gpu.graphics_qfi;
  }
  swapchain_create_info.preTransform = FLAGGED(surface_capabilities.supportedTransforms,
                                               VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                                     ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                                     : surface_capabilities.currentTransform;
  // Find supported composite alpha mode
  // Prefer: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
  // Default: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
  VkCompositeAlphaFlagBitsKHR composite_alpha;
  for (composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
       composite_alpha < VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR &&
       !FLAGGED(surface_capabilities.supportedCompositeAlpha, composite_alpha);
       composite_alpha <<= 1);
  swapchain_create_info.compositeAlpha = composite_alpha;
  swapchain_create_info.presentMode = vk_env.gpu.present_mode;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = old_swapchain;
  VK_CALL(vkCreateSwapchainKHR(vk_env.device, &swapchain_create_info, NULL, &vk_env.swapchain));
  LOG_DEBUG_INFO("Created new swapchain");

  if (old_swapchain) {
    destroy_swapchain(old_swapchain);
    LOG_DEBUG_INFO("Destroyed old swapchain");
  }

  // Have to call vkGetSwapchainImagesKHR twice, although we already know the value of num_buffers,
  // to prevent UNASSIGNED-CoreValidation-SwapchainInvalidCount
  VK_CALL(vkGetSwapchainImagesKHR(vk_env.device, vk_env.swapchain, &vk_env.gpu.num_buffers, NULL));
  vk_env.swapchain_images = halloc_type(VkImage, vk_env.gpu.num_buffers);
  VK_CALL(vkGetSwapchainImagesKHR(vk_env.device, vk_env.swapchain, &vk_env.gpu.num_buffers, vk_env.swapchain_images));
  vk_env.swapchain_views = halloc_type(VkImageView, vk_env.gpu.num_buffers);
  vk_env.framebuffers = halloc_type(VkFramebuffer, vk_env.gpu.num_buffers);

  VkImageView attachments[1] = { VK_NULL_HANDLE };
  VkFramebufferCreateInfo create_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
  create_info.renderPass = vk_env.render_pass;
  create_info.attachmentCount = 1;
  create_info.pAttachments = attachments;
  create_info.width = vk_env.window->width;
  create_info.height = vk_env.window->height;
  create_info.layers = 1;
  for (uint32_t i = 0; i < vk_env.gpu.num_buffers; i++) {
    VK_CALL(create_image_view(
      vk_env.device,
      vk_env.swapchain_images[i],
      vk_env.gpu.surface_format.format,
      VK_IMAGE_ASPECT_COLOR_BIT,
      &vk_env.swapchain_views[i]
    ));
    attachments[0] = vk_env.swapchain_views[i];
    VK_CALL(vkCreateFramebuffer(vk_env.device, &create_info, NULL, &vk_env.framebuffers[i]));
    // ERROR Validation Error : [VUID - VkFramebufferCreateInfo - width - 00885]
    // Object 0 : handle = 0x1ddce66c638, type = VK_OBJECT_TYPE_DEVICE; | MessageID = 0xb6981526 |
    // vkCreateFramebuffer() : Requested VkFramebufferCreateInfo width must be greater than zero.
  }
  vk_env.frame_index = 0;
  LOG_DEBUG_INFO("Created %d swapchain images, views and framebuffers", vk_env.gpu.num_buffers);

  LOG_DEBUG_INFO("End create_swapchain()");
}

void destroy_swapchain_final() {
  destroy_swapchain(vk_env.swapchain);
  LOG_DEBUG_INFO("Destroyed swapchain");
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
  push_create(create_sync_objects, destroy_sync_objects);
  push_create(create_render_pass, destroy_render_pass);
  push_create(create_shader_modules, destroy_shader_modules);
  push_create(create_vertex_buffer, destroy_vertex_buffer);
  push_create(create_uniform_buffer, destroy_uniform_buffer);
  push_create(create_layouts, destroy_layouts);
  push_create(create_descriptor_pool, destroy_descriptor_pool);
  push_create(alloc_descriptor_sets, free_descriptor_sets);
  push_create(create_pipeline_cache, destroy_pipeline_cache);
  push_create(create_pipeline, destroy_pipeline);
  push_create(NULL, destroy_swapchain_final);

  vk_env.initialized = true;

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

void resize() {
  if (!vk_env.initialized)
    return;
  create_swapchain();
}

void move(int x, int y) {
  mvp[12] += x * 0.01f;
  mvp[13] += y * 0.01f;
}

VkResult handle_swapchain_result(VkResult ve) {
  switch (ve) {
    case VK_ERROR_OUT_OF_DATE_KHR:
      // Window was resized
      resize();
      break;
    case VK_SUCCESS:
      // Swapchain is all good
    case VK_SUBOPTIMAL_KHR:
      // Swapchain is in suboptimal state but still able to present the image
      return VE_OK;
    case VK_ERROR_SURFACE_LOST_KHR:
      vkDestroySurfaceKHR(vk_env.instance, vk_env.surface, NULL);
      create_surface();
      resize();
      break;
#ifdef _DEBUG
    default:
      LOG_VK_ERROR(vkAcquireNextImageKHR, ve);
#endif
  }
  return ve;
}

void begin_render() {
  // Ensure no more than FRAME_LAG renderings are outstanding
  vkWaitForFences(vk_env.device, 1, &vk_env.fences[vk_env.frame_index], VK_TRUE, UINT64_MAX);
  vkResetFences(vk_env.device, 1, &vk_env.fences[vk_env.frame_index]);

  // Get index of next available swapchain image
  while (handle_swapchain_result(vkAcquireNextImageKHR(
    vk_env.device, vk_env.swapchain, UINT64_MAX,
    vk_env.image_acquired_semaphores[vk_env.frame_index],
    VK_NULL_HANDLE, &vk_env.current_buffer)
  ));
}

void end_render() {
  // Wait for image acquired semaphore to be signaled
  // Then submit image to graphics queue
  VkPipelineStageFlags pipeline_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &vk_env.image_acquired_semaphores[vk_env.frame_index];
  submit_info.pWaitDstStageMask = &pipeline_stage_mask;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &vk_env.command_buffers[vk_env.current_buffer];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &vk_env.draw_complete_semaphores[vk_env.frame_index];
  VK_CALL(vkQueueSubmit(vk_env.graphics_queue, 1, &submit_info, vk_env.fences[vk_env.frame_index]));

  // Wait for draw complete (or image ownership)
  VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = vk_env.distinct_qfi
                               ? &vk_env.image_ownership_semaphores[vk_env.frame_index]
                               : &vk_env.draw_complete_semaphores[vk_env.frame_index];
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &vk_env.swapchain;
  present_info.pImageIndices = &vk_env.current_buffer;
  handle_swapchain_result(vkQueuePresentKHR(vk_env.present_queue, &present_info));
  vk_env.frame_index = (vk_env.frame_index + 1) % vk_env.frame_lag;
}

void buffer_commands(uint32_t buffer_index) {
  VkCommandBufferBeginInfo cmd_begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  VkCommandBuffer command_buffer = vk_env.command_buffers[buffer_index];
  vkResetCommandBuffer(command_buffer, 0);
  VK_CALL(vkBeginCommandBuffer(command_buffer, &cmd_begin_info));

  VkRenderPassBeginInfo rp_begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
  rp_begin_info.renderPass = vk_env.render_pass;
  rp_begin_info.framebuffer = vk_env.framebuffers[buffer_index];
  rp_begin_info.renderArea.extent.width = vk_env.window->width;
  rp_begin_info.renderArea.extent.height = vk_env.window->height;
  const VkClearValue clear_values[] = {
    { { 0.0f, 0.0f, 0.1f, 1.0f } } // color
  };
  rp_begin_info.clearValueCount = ARRAY_COUNT(clear_values);
  rp_begin_info.pClearValues = clear_values;
  vkCmdBeginRenderPass(command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_env.pipeline);
  // Vertex buffer
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vk_env.rect_vb.buffer, &offset);
  // Uniform buffer
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vk_env.pipeline_layout, 0, 1,
                          &vk_env.descriptor_sets[buffer_index], 0, NULL);
  // Viewport
  VkViewport viewport = { 0 };
  float viewport_dimension;
  if (vk_env.window->width > vk_env.window->height) {
    viewport_dimension = (float)vk_env.window->height;
    viewport.x = (vk_env.window->width - vk_env.window->height) / 2.0f;
  }
  else {
    viewport_dimension = (float)vk_env.window->width;
    viewport.y = (vk_env.window->height - vk_env.window->width) / 2.0f;
  }
  viewport.width = viewport_dimension;
  viewport.height = -viewport_dimension;
  viewport.y += viewport_dimension;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  // Scissor
  const VkRect2D scissor = {
    { 0, 0 }, // offset
    { vk_env.window->width, vk_env.window->height } // extent
  };
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  vkCmdDraw(command_buffer, ARRAY_COUNT(rect), 1, 0, 0);

  // Note that ending the renderpass changes the image's layout from
  // COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
  vkCmdEndRenderPass(command_buffer);
  VK_CALL(vkEndCommandBuffer(command_buffer));
}

void render() {
  if (vk_env.window->minimized)
    return;
  begin_render(vk_env);
  memcpy(vk_env.mvp_ub.mem_ptr, mvp, sizeof mvp);
  buffer_commands(vk_env.current_buffer);
  end_render(vk_env);
}

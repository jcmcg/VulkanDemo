#pragma once

#include <vulkan/vulkan.h>
#include <stdbool.h>

#define APP_NAME "VulkanDemo"
#define APP_VERSION VK_MAKE_VERSION(1, 0, 0)
#define ENGINE_NAME "TestEngine"
#define ENGINE_VERSION VK_MAKE_VERSION(1, 0, 0)
#define SHADER_NAME "shader"
#define SHADER_ENTRY_POINT_NAME "main"

#define ARRAY_COUNT(a) (sizeof (a) / sizeof (a[0]))
#define CLAMP(V, MIN, MAX) if (V < MIN) V = MIN; else if (MAX && V > MAX) V = MAX
#define FLAGGED(g, f) (((g) & (f)) == (f))

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

#define NUM_BUFFERS 3 // Triple buffering

typedef enum {
  VE_OK,
  VE_NO_INSTANCE_LAYERS,
  VE_INSTANCE_LAYER_UNAVAILABLE,
  VE_NO_INSTANCE_EXTENSIONS,
  VE_INSTANCE_EXTENSION_UNAVAILABLE,
  VE_NO_PHYSICAL_DEVICES,
  VE_NO_SUPPORTED_PHYSICAL_DEVICE,
  VE_SHADER_FILE_OPEN,
  VE_SHADER_FILE_READ,
  VE_NO_SURFACE_FORMATS,
  VE_NO_SUITABLE_SURFACE_FORMAT,
  VE_NO_PRESENT_MODES,
  VE_NO_SUITABLE_PRESENT_MODE,
  VE_NO_SUITABLE_DEPTH_FORMAT,
  VE_NO_SUITABLE_TEXTURE_FORMAT
} VULKAN_ERROR;

typedef struct cds_entry_s cds_entry_t;
typedef struct cds_entry_s {
  void (*destroy)();
  cds_entry_t *next;
} cds_entry_t;

typedef struct window_s window_t;

typedef enum {
  GPU_SUPPORT_NONE,
  GPU_SUPPORT_RASTERIZATION = 1,
  GPU_SUPPORT_RAYTRACING = 2,
  GPU_SUPPORT_TEXTURE_COMPRESSION = 4,
  GPU_SUPPORT_ANISTROPIC_FILTERING = 8
} GPU_SUPPORT;

typedef struct {
  VkPhysicalDevice device;
  char *name;
  uint32_t graphics_qfi;
  uint32_t present_qfi;
  GPU_SUPPORT support;
  VkSurfaceFormatKHR surface_format;
  uint32_t num_buffers;
  VkSampleCountFlagBits num_aa_samples;
  VkPresentModeKHR present_mode;
  VkFormat depth_format;
  VkFormat texture_format;
  VkPhysicalDeviceMemoryProperties memory_properties;
} GPU;

typedef struct {
  VkBuffer buffer;
  VkDeviceMemory device_memory;
} VkVertexBuffer;

typedef struct {
  VkBuffer buffer;
  VkDeviceMemory device_memory;
  void *mem_ptr;
} VkUniformBuffer;

typedef struct vk_env_s {
  VULKAN_ERROR error;
  bool initialized;
  cds_entry_t *cd_stack;
  VkInstance instance;
#ifdef _DEBUG
  VkDebugUtilsMessengerEXT debug_messenger;
#endif
  window_t *window;
  VkSurfaceKHR surface;
  GPU gpu;
  VkQueue graphics_queue;
  VkQueue present_queue;
  bool distinct_qfi;
  VkDevice device;
  VkCommandPool command_pool;
  VkCommandBuffer *command_buffers;
  VkFence *fences;
  VkSemaphore *image_acquired_semaphores;
  VkSemaphore *image_ownership_semaphores;
  VkSemaphore *draw_complete_semaphores;
  uint32_t frame_lag;
  VkRenderPass render_pass;
  VkShaderModule vertex_shader;
  VkShaderModule fragment_shader;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet *descriptor_sets;
  VkDescriptorSetLayout descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipelineCache pipeline_cache;
  VkPipeline pipeline;
  VkSwapchainKHR swapchain;
  VkImage *swapchain_images;
  VkImageView *swapchain_views;
  VkFramebuffer *framebuffers;
  VkVertexBuffer rect_vb;
  VkUniformBuffer mvp_ub;
  uint32_t frame_index;
  uint32_t current_buffer;
} vk_env_t;

vk_env_t vk_env;

typedef struct {
  float x, y, z;
} vec3_t;

typedef struct {
  vec3_t position;
  vec3_t colour;
} vertex_t;

void init_vulkan();
void cleanup_vulkan();
void resize();
void render();

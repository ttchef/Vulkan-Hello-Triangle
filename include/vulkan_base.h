
#ifndef VULKAN_BASE_H
#define VULKAN_BASE_H

#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    VkQueue queue;
    uint32_t familyIndex;
} VulkanQueue;

typedef struct {
    VkSwapchainKHR swapchain;

    VkImage* images;
    VkImageView* imageViews;
    uint32_t imagesCount;

    uint32_t width;
    uint32_t height;
    VkFormat format;
} VulkanSwapchain;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout layout;
} VulkanPipeline;

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory;
} VulkanBuffer;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkDevice device;
    VulkanQueue graphicsQueue;
} VulkanContext;

VulkanContext* initVulkan(uint32_t glfwExtensionCount, const char** glfwExtensions,
        uint32_t deviceExtensionCount, const char** deviceExtensions);

// vulkan_device.c
bool initVulkanInstance(VulkanContext* context, uint32_t glfwExtensionCount, const char** glfwExtensions);
bool selectPhysicalDevice(VulkanContext* context);
bool createLogicalDevice(VulkanContext* context, uint32_t deviceExtensionCount, const char** deviceExtensions);
void exitVulkan(VulkanContext* context);

// vulkan_swapchain.c 
VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

// vulkan_renderpass.c 
VkRenderPass createRenderPass(VulkanContext* context, VkFormat format);
void destroyRenderPass(VulkanContext* context, VkRenderPass renderPass);

// vulkan_pipeline.c 
VkShaderModule createShaderModule(VulkanContext* context, const char* filepath);
VulkanPipeline createPipeline(VulkanContext* context, const char* vertPath, const char* fragPath,
        VkRenderPass renderPass, uint32_t width, uint32_t height,
        VkVertexInputAttributeDescription* attributes, uint32_t numAttributes,
        VkVertexInputBindingDescription* binding);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);

// vulkan_utils.c 
void createBuffer(VulkanContext* context, VulkanBuffer* buffer, uint64_t size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer);
uint32_t findMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties);

#endif // VULKAN_BASE_H
      


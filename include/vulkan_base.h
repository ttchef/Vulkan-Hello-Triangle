
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
    uint32_t width;
    uint32_t height;
    VkFormat format;
} VulkanSwapchain;

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
VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

#endif // VULKAN_BASE_H
      


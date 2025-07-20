
#ifndef VULKAN_BASE_H
#define VULKAN_BASE_H

#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
} VulkanContext;

VulkanContext* initVulkan(uint32_t glfwExtensionCount, const char** glfwExtensions);
bool initVulkanInstance(VulkanContext* context, uint32_t glfwExtensionCount, const char** glfwExtensions);
bool selectPhysicalDevice(VulkanContext* context);

#endif // VULKAN_BASE_H
      


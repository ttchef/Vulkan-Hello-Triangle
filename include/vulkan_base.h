
#ifndef VULKAN_BASE_H
#define VULKAN_BASE_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

typedef struct {
    VkInstance instance;
} VulkanContext;

VulkanContext* initVulkan(uint32_t glfwExtensionCount, const char** glfwExtensions);
bool initVulkanInstance(VulkanContext* context, uint32_t glfwExtensionCount, const char** glfwExtensions);

#endif // VULKAN_BASE_H
      


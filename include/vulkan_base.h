
#ifndef VULKAN_BASE_H
#define VULKAN_BASE_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

typedef struct {
    VkInstance instance;
} VulkanContext;

VulkanContext* initVulkan();
bool initVulkanInstance(VulkanContext* context);

#endif // VULKAN_BASE_H
      


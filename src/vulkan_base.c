
#include <stdio.h>
#include <stdlib.h>

#include "../include/vulkan_base.h"

bool initVulkanInstance(VulkanContext* context) {

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Mef";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Mef_Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledLayerNames = NULL;

    if (vkCreateInstance(&createInfo, NULL, &context->instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vulkan instance\n");
        return false;
    }

    return true;
}

VulkanContext* initVulkan() {
    VulkanContext* context = malloc(sizeof(VulkanContext));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for vulkan context!\n");
        return NULL;
    }

    if (!initVulkanInstance(context)) {
        return NULL;
    }

    return context;
}



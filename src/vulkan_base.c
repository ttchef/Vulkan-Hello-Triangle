
#include <stdio.h>
#include <stdlib.h>

#include "../include/vulkan_base.h"

#include <darray/darray.h>

bool initVulkanInstance(VulkanContext* context, uint32_t glfwExtensionCount, const char** glfwExtensions) {

    // get layers
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties* layerProperties = malloc(sizeof(VkLayerProperties) * layerCount);
    if (!layerProperties) {
        fprintf(stderr, "Failed to allocate layerProperties!\n");
        return false;
    }
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

    const char* enabledLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    
    // check if we have the layer
    for (uint32_t i = 0; i < ARRAY_COUNT(enabledLayers); i++) {
        bool found = false;
        for (uint32_t j = 0; j < layerCount; j++) {
            if (memcmp(enabledLayers[i], layerProperties[j].layerName, strlen(layerProperties[j].layerName)) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "Layer %s hasnt been found!\n", enabledLayers[i]);
            return false;
        }
    }

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

    createInfo.enabledLayerCount = ARRAY_COUNT(enabledLayers);
    createInfo.ppEnabledLayerNames = enabledLayers;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    if (vkCreateInstance(&createInfo, NULL, &context->instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vulkan instance!\n");
        return false;
    }

    return true;
}

VulkanContext* initVulkan(uint32_t glfwExtensionCount, const char** glfwExtensions) {
    VulkanContext* context = malloc(sizeof(VulkanContext));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for vulkan context!\n");
        return NULL;
    }

    if (!initVulkanInstance(context, glfwExtensionCount, glfwExtensions)) {
        return NULL;
    }

    return context;
}



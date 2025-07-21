
#include <stdio.h>
#include <stdlib.h>

#include "../include/vulkan_base.h"

#include <darray/darray.h>
#include <vulkan/vulkan_core.h>

bool createLogicalDevice(VulkanContext *context, uint32_t deviceExtensionCount, const char** deviceExtensions) {
    
    // Queues
    uint32_t numQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, NULL);

    VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies);

    uint32_t graphicsQueueIndex = 0;
    for (uint32_t i = 0; i < numQueueFamilies; i++) {
        VkQueueFamilyProperties queueFamily = queueFamilies[i];;
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueIndex = i;
            break;
        }
    }

    free(queueFamilies);

    float priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = priorities;

    VkPhysicalDeviceFeatures enabledFeatures = {0};

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pEnabledFeatures = &enabledFeatures;
    createInfo.enabledExtensionCount = deviceExtensionCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    
    if (vkCreateDevice(context->physicalDevice, &createInfo, NULL, &context->device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device!\n");
        return false;
    }

    // Acquire queues
    context->graphicsQueue.familyIndex = graphicsQueueIndex;
    vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue);

    return true;
}

bool selectPhysicalDevice(VulkanContext *context) {
    uint32_t numDevices;
    vkEnumeratePhysicalDevices(context->instance, &numDevices, NULL);
    if (numDevices == 0) {
        fprintf(stderr, "Failed to find a fitting vulkan GPU");
        context->physicalDevice = 0;
        return false;
    }

    VkPhysicalDevice* physicalDevices = malloc(sizeof(VkPhysicalDevice) * numDevices);
    if (!physicalDevices) {
        fprintf(stderr, "Failed to allocate memory for all physical devices!\n");
        context->physicalDevice = 0;
        return false;
    }

    vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices);
    printf("Found: %d GPUs\n", numDevices);

    for (uint32_t i = 0; i < numDevices; i++) {
        VkPhysicalDeviceProperties deviceProperties = {0};
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        printf("Found GPU: %s\n", deviceProperties.deviceName);
    }

    VkPhysicalDeviceProperties selectedDeviceProperties = {0};
    vkGetPhysicalDeviceProperties(physicalDevices[0], &selectedDeviceProperties);
    printf("Will select %s GPU!\n", selectedDeviceProperties.deviceName);
    context->physicalDevice = physicalDevices[0];
    context->physicalDeviceProperties = selectedDeviceProperties;

    free(physicalDevices);

    return true;
}

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
    
    free(layerProperties);

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

VulkanContext* initVulkan(uint32_t glfwExtensionCount, const char** glfwExtensions, uint32_t deviceExtensionCount, const char** deviceExtensions) {
    VulkanContext* context = malloc(sizeof(VulkanContext));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for vulkan context!\n");
        return NULL;
    }

    if (!initVulkanInstance(context, glfwExtensionCount, glfwExtensions)) return NULL;
    if (!selectPhysicalDevice(context)) return NULL;
    if (!createLogicalDevice(context, deviceExtensionCount, deviceExtensions)) return NULL;

    return context;
}


void exitVulkan(VulkanContext *context) {
    // wait for graphics crad to finish work
    vkDeviceWaitIdle(context->device);
    vkDestroyDevice(context->device, NULL);
    vkDestroyInstance(context->instance, NULL);
}



#include <stdio.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// own libs
#include <darray/darray.h>
#include <drings/drings.h>

#include "../include/vulkan_base.h"

int main() {

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize glfw!\n");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan C yeahh", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create glfw window\n");
        glfwTerminate();
        return -1;
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    const char* enabledDeviceExtensions[] = {
        "VK_KHR_swapchain"
    };

    VulkanContext* context = initVulkan(glfwExtensionCount, glfwExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);
    if (!context) {
        fprintf(stderr, "Failed to create vulkan context!\n");
        return -1;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(context->instance, window, NULL, &surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface!\n");
        return -1;
    }

    VulkanSwapchain swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    destroySwapchain(context, &swapchain);
    vkDestroySurfaceKHR(context->instance, surface, NULL);
    exitVulkan(context);
    free(context);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}



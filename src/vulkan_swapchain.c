
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "../include/vulkan_base.h"

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage) {
    VulkanSwapchain result = {0};

    VkBool32 supportsPresent = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex,
            surface, &supportsPresent);

    if (!supportsPresent) {
        fprintf(stderr, "Graphics queue does not support present!\n");
        return result;
    }

    // getting supported image formats
    uint32_t numFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, NULL);

    if (numFormats <= 0) {
        fprintf(stderr, "No surface image formats available!\n");
        return result;
    }

    VkSurfaceFormatKHR* availableFormats = malloc(sizeof(VkSurfaceFormatKHR) * numFormats);
    if (!availableFormats) {
        fprintf(stderr, "Failed to allocate memory for available image formats!\n");
        return result;
    }

    vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, availableFormats);

    VkFormat format = availableFormats[0].format;
    VkColorSpaceKHR colorSpace = availableFormats[0].colorSpace;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities);
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
        surfaceCapabilities.currentExtent.width = surfaceCapabilities.maxImageExtent.width;
        surfaceCapabilities.currentExtent.height = surfaceCapabilities.maxImageExtent.height;
    }

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = 3;
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = usage;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        createInfo.preTransform = surfaceCapabilities.currentTransform;
    }

        
    if (vkCreateSwapchainKHR(context->device, &createInfo, NULL, &result.swapchain) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vulkan swapchain!\n");
        return result;
    }

    result.format = format;
    result.width = surfaceCapabilities.currentExtent.width;
    result.height = surfaceCapabilities.currentExtent.height;

    uint32_t numImages = 0;
    vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, NULL);
    result.imagesCount = numImages;

    // Acquire swapchain images 
    result.images = malloc(sizeof(VkImage) * numImages);
    if (!result.images) {
        fprintf(stderr, "Failed to allocate memory for swapchain images!\n");
        return result;
    }

    vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, result.images);

    // create image views 
    result.imageViews = malloc(sizeof(VkImageView) * numImages);
    if (!result.imageViews) {
        fprintf(stderr, "Failed to allocate memory for image views!\n");
        return result;
    }
    for (uint32_t i = 0; i < numImages; i++) {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = result.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.components = (VkComponentMapping){0};
        createInfo.subresourceRange = (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        if (vkCreateImageView(context->device, &createInfo, NULL, &result.imageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image views!\n");
            return result;
        }

    }

    free(availableFormats);
    return result;
}

void destroySwapchain(VulkanContext *context, VulkanSwapchain *swapchain) {
    
    if (swapchain->images) {
        free(swapchain->images);
        swapchain->images = NULL;
    }
    
    for (uint32_t i = 0; i < swapchain->imagesCount; i++) {
        vkDestroyImageView(context->device, swapchain->imageViews[i], NULL);
    }

    if (swapchain->imageViews) {
        free(swapchain->imageViews);
        swapchain->imageViews = NULL;
    }

    vkDestroySwapchainKHR(context->device, swapchain->swapchain, 0);
}


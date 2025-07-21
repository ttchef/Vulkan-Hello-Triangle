
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// own libs
#include <darray/darray.h>
#include <drings/drings.h>

#include "../include/vulkan_base.h"

VulkanContext* context;
VkSurfaceKHR surface;
VulkanSwapchain swapchain;
VkRenderPass renderPass;
VkFramebuffer* framebuffers;
VkCommandPool commandPool;
VkCommandBuffer commandBuffer;
VkFence fence;
VulkanPipeline pipeline;

void initApplication(GLFWwindow* window) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    const char* enabledDeviceExtensions[] = {
        "VK_KHR_swapchain"
    };

    context = initVulkan(glfwExtensionCount, glfwExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);
    if (!context) {
        fprintf(stderr, "Failed to create vulkan context!\n");
        return;
    }

    surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(context->instance, window, NULL, &surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface!\n");
        return;
    }

    swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    renderPass = createRenderPass(context, swapchain.format);
    framebuffers = malloc(sizeof(VkFramebuffer) * swapchain.imagesCount);
    if (!framebuffers) {
        fprintf(stderr, "Failed to allocate memory for framebuffers!\n");
        return;
    }

    for (uint32_t i = 0; i < swapchain.imagesCount; i++) {
        VkFramebufferCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapchain.imageViews[i];
        createInfo.width = swapchain.width;
        createInfo.height = swapchain.height;
        createInfo.layers = 1;

        if (vkCreateFramebuffer(context->device, &createInfo, NULL, &framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create vulkan framebuffers!\n");
            return;
        }
    }   

    pipeline = createPipeline(context, "shaders/triangle_vert.spv", "shaders/triangle_frag.spv", renderPass, swapchain.width, swapchain.height);

    {
        VkFenceCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if (vkCreateFence(context->device, &createInfo, NULL, &fence) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create fence!\n");
            return;
        }

    }

    {

        VkCommandPoolCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;

        if (vkCreateCommandPool(context->device, &createInfo, NULL, &commandPool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create Commandpool!\n");
            return;
        }
    }

    {
    
        VkCommandBufferAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = commandPool;

        if (vkAllocateCommandBuffers(context->device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create commandbuffer!\n");
            return;
        }
    }
}

void renderApplication() {

    static float greenChannel = 0.0f;
    greenChannel += 0.01f;
    if (greenChannel > 1.0f) greenChannel = 0.0f;

    uint32_t imageIndex = 0;

    // getting image from swapchain
    vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, 0, fence, &imageIndex);

    // reset command pool
    if (vkResetCommandPool(context->device, commandPool, 0) != VK_SUCCESS) {
        fprintf(stderr, "Failed to reset commandpool!\n");
        return;
    }

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin commandbuffer!\n");
        return;
    }

    {
        VkClearValue clearValue = {
            .color = { {1.0f, greenChannel, 1.0, 1.0f} }
        };

        VkRenderPassBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = renderPass;
        beginInfo.framebuffer = framebuffers[imageIndex];
        beginInfo.renderArea = (VkRect2D){ (VkOffset2D){0, 0}, (VkExtent2D){swapchain.width, swapchain.height}};
        beginInfo.clearValueCount = 1;
        beginInfo.pClearValues = &clearValue;
            
        vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record commandbuffer!\n");
        return;
    }

    if (vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        fprintf(stderr, "Failed to wait for fences!\n");
        return;
    }
    if (vkResetFences(context->device, 1, &fence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to reset fences!\n");
        return;
    }

    // send to graphicsQueue
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, 0);
 

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);
}

void shutdownApplication() {
    vkDeviceWaitIdle(context->device);

    vkDestroyFence(context->device, fence, NULL);
    vkDestroyCommandPool(context->device, commandPool, NULL);

    destroyPipeline(context, &pipeline);

    for (uint32_t i = 0; i < swapchain.imagesCount; i++) {
        vkDestroyFramebuffer(context->device, framebuffers[i], NULL);
    }

    destroyRenderPass(context, renderPass);
    destroySwapchain(context, &swapchain);
    vkDestroySurfaceKHR(context->instance, surface, NULL);
    exitVulkan(context);
    free(framebuffers);
    free(context);

}

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

    initApplication(window);

    while (!glfwWindowShouldClose(window)) {
        renderApplication();
        glfwPollEvents();
    }

    shutdownApplication();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}



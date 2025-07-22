
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// own libs
#include <darray/darray.h>
#include <drings/drings.h>

#include "../include/vulkan_base.h"

#define FRAMES_IN_FLIGHT 2

GLFWwindow* window;

VulkanContext* context;
VkSurfaceKHR surface;
VulkanSwapchain swapchain;
VkRenderPass renderPass;
VkFramebuffer* framebuffers;
VkCommandPool commandPools[FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
VkFence fences[FRAMES_IN_FLIGHT];
VulkanPipeline pipeline;
VkSemaphore acrquireSemaphores[FRAMES_IN_FLIGHT];
VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];

uint32_t frameIndex = 0;

bool framebufferResized = false;

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    framebufferResized = true;
}

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

    swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0);

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

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++){
        VkFenceCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(context->device, &createInfo, NULL, &fences[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create fence!\n");
            return;
        }
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkSemaphoreCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(context->device, &createInfo, NULL, &acrquireSemaphores[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create semaphore!\n");
            exit(1);
        }
        if (vkCreateSemaphore(context->device, &createInfo, NULL, &releaseSemaphores[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create semaphore!\n");
            exit(1);
        }
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkCommandPoolCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;

        if (vkCreateCommandPool(context->device, &createInfo, NULL, &commandPools[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create Commandpool!\n");
            return;
        }
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
    
        VkCommandBufferAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = commandPools[i];

        if (vkAllocateCommandBuffers(context->device, &allocInfo, &commandBuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create commandbuffer!\n");
            return;
        }
    }
}

void recreateRenderPass() {
    if (renderPass) {
        destroyRenderPass(context, renderPass);

        for (uint32_t i = 0; i < swapchain.imagesCount; i++) {
            vkDestroyFramebuffer(context->device, framebuffers[i], NULL);
        }
    }

    renderPass = createRenderPass(context, swapchain.format);

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

}

void recreateSwapchain() {

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(context->device);

    VulkanSwapchain oldSwapchain = swapchain;
    swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &oldSwapchain);

    destroySwapchain(context, &oldSwapchain);

    recreateRenderPass();
}

void renderApplication() {

    static float greenChannel = 0.0f;
    greenChannel = (sin(glfwGetTime()) + 1) / 2;
    if (greenChannel > 1.0f) greenChannel = 0.0f;

    uint32_t imageIndex = 0;

    if (vkWaitForFences(context->device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        fprintf(stderr, "Failed to wait for fences!\n");
        return;
    }

    // getting image from swapchain
    VkResult result = vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acrquireSemaphores[frameIndex], 0, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchain();
        fprintf(stderr, "Resize!\n");
        return;
    }
    else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to acrquire next image from swapchain!\n");
        return;
    }

    if (vkResetFences(context->device, 1, &fences[frameIndex]) != VK_SUCCESS) {
        fprintf(stderr, "Failed to reset fences!\n");
        return;
    }



    // reset command pool
    if (vkResetCommandPool(context->device, commandPools[frameIndex], 0) != VK_SUCCESS) {
        fprintf(stderr, "Failed to reset commandpool!\n");
        return;
    }

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffers[frameIndex], &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin commandbuffer!\n");
        return;
    }

    
    {
        VkCommandBuffer commandBuffer = commandBuffers[frameIndex];

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

        VkViewport viewport = (VkViewport){0.0f, 0.0f, (float)swapchain.width, (float)swapchain.height};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = (VkRect2D){{0, 0}, {swapchain.width, swapchain.height}};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
    }

    if (vkEndCommandBuffer(commandBuffers[frameIndex]) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record commandbuffer!\n");
        return;
    }

    // send to graphicsQueue
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[frameIndex];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acrquireSemaphores[frameIndex];

    VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submitInfo.pWaitDstStageMask = &waitMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphores[frameIndex];
    vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fences[frameIndex]);

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphores[frameIndex];

    result = vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapchain();
        fprintf(stderr, "Resize!\n");
    }
    else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to acrquire next image from swapchain!\n");
    }


    frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void shutdownApplication() {
    vkDeviceWaitIdle(context->device);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(context->device, fences[i], NULL);
        vkDestroySemaphore(context->device, acrquireSemaphores[i], NULL);
        vkDestroySemaphore(context->device, releaseSemaphores[i], NULL);

    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(context->device, commandPools[i], NULL);
    }

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
    window = glfwCreateWindow(800, 600, "Vulkan C yeahh", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create glfw window\n");
        glfwTerminate();
        return -1;
    }

    //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

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



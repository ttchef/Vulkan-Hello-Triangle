
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../vendor/stb/stb_image.h"

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
VulkanBuffer vertexBuffer;
VulkanBuffer indexBuffer;
VulkanImage image;

VkSampler sampler;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;
VkDescriptorSetLayout descriptorLayout;

uint32_t frameIndex = 0;
bool framebufferResized = false;

float vertexData[] = {
    0.5f, -0.5f,        // Pos
    1.0f, 0.0f, 0.0f,   // Color
    1.0f, 0.0f,         // UV

    0.5f, 0.5f,
    0.0f, 1.0f, 0.0f,
    1.0f, 1.0f,

    -0.5f, 0.5f,
    0.0f, 0.0f, 1.0f, 
    0.0f, 1.0f,

    -0.5f, -0.5f, 
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f
};

uint32_t indexData[] = {
    0, 1, 2,
    3, 0, 2
};

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    framebufferResized = true;
}

void initApplication(GLFWwindow* window) {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    const char* additionalInstanceExtensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
    };

    uint32_t totalInstanceExtensionCount = glfwExtensionCount + ARRAY_COUNT(additionalInstanceExtensions);
    const char** enabledInstanceExtensions = malloc(sizeof(char*) * totalInstanceExtensionCount);
    if (!enabledInstanceExtensions) {
        fprintf(stderr, "Failed to allocate memory for enabledInstanceExtensions!\n");
        return;
    }

    // copy glfw
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        enabledInstanceExtensions[i] = glfwExtensions[i];
    }

    // copy additional
    for (uint32_t i = 0; i < ARRAY_COUNT(additionalInstanceExtensions); i++) {
        enabledInstanceExtensions[glfwExtensionCount + i] = additionalInstanceExtensions[i];
    }

    const char* enabledDeviceExtensions[] = {
        "VK_KHR_swapchain"
    };

    context = initVulkan(totalInstanceExtensionCount, enabledInstanceExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);
    if (!context) {
        fprintf(stderr, "Failed to create vulkan context!\n");
        return;
    }
    free(enabledInstanceExtensions);
    enabledInstanceExtensions = NULL;

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
            exit(-1);
        }
    }   

    {
        VkSamplerCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.magFilter = VK_FILTER_NEAREST;
        createInfo.minFilter = VK_FILTER_NEAREST;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeV = createInfo.addressModeU;
        createInfo.addressModeW = createInfo.addressModeU;
        createInfo.mipLodBias = 0.0f;
        createInfo.maxAnisotropy = 1.0f;
        createInfo.minLod = 0.0f;
        createInfo.maxLod = 1.0f;

        if (vkCreateSampler(context->device, &createInfo, NULL, &sampler) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create sampler!\n");
            exit(-1);
        }
    }

    {

        const char* path = "/home/ttchef/coding/c/Vulkan-Hello-Triangle/res/images/forest.png";
        int width, height, channels;
        uint8_t* data = stbi_load(path, &width, &height, &channels, 4);
        if (!data) {
            fprintf(stderr, "Failed to load image data: %s\n", path);
            exit(-1);
        }

        createImage(context, &image, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
        uploadDataToImage(context, &image, data, width * height * 4,
                          width, height, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        stbi_image_free(data);
    }

    {

        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        };

        VkDescriptorPoolCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.maxSets = 1;
        createInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
        createInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(context->device, &createInfo, NULL, &descriptorPool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create descriptorPool!\n");
            exit(-1);
        }
    }

    {
        VkDescriptorSetLayoutBinding bindings[] = {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, 0 },
        };


        VkDescriptorSetLayoutCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = ARRAY_COUNT(bindings);
        createInfo.pBindings = bindings;

        if (vkCreateDescriptorSetLayout(context->device, &createInfo, NULL, &descriptorLayout) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create descriptor layout!\n");
            exit(-1);
        }

        VkDescriptorSetAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorLayout;

        if (vkAllocateDescriptorSets(context->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            fprintf(stderr, "Failed to allocate descriptor set!\n");
            exit(-1);
        }

        VkDescriptorImageInfo imageInfo = {0};
        imageInfo.sampler = sampler;
        imageInfo.imageView = image.view;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet descriptorWrites[1];
        descriptorWrites[0] = (VkWriteDescriptorSet){0};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(context->device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, NULL);
    }

    VkVertexInputAttributeDescription vertexAttributeDescriptions[3] = {0};
    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescriptions[0].offset = 0;

    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = sizeof(float) * 2;

    vertexAttributeDescriptions[2].binding = 0;
    vertexAttributeDescriptions[2].location = 2;
    vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescriptions[2].offset = sizeof(float) * 5;

    VkVertexInputBindingDescription vertexInputBinding = {0};
    vertexInputBinding.binding = 0;
    vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBinding.stride = sizeof(float) * 7;



    pipeline = createPipeline(context, "shaders/texture_vert.spv", "shaders/texture_frag.spv", renderPass, swapchain.width,
            swapchain.height, vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions),
            &vertexInputBinding);

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

    createBuffer(context, &vertexBuffer, sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    uploadDataToBuffer(context, &vertexBuffer, vertexData, sizeof(vertexData));


    createBuffer(context, &indexBuffer, sizeof(indexData), VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    uploadDataToBuffer(context, &indexBuffer, indexData, sizeof(indexData));


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
        fprintf(stderr, "Resize!\n");
        framebufferResized = false;
        recreateSwapchain();
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
    
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);

        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(indexData), 1, 0, 0, 0);

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
        fprintf(stderr, "Resize!\n");
        framebufferResized = false;
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to acrquire next image from swapchain!\n");
    }


    frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void shutdownApplication() {
    vkDeviceWaitIdle(context->device);

    destroyImage(context, &image);

    destroyBuffer(context, &indexBuffer);
    destroyBuffer(context, &vertexBuffer);

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(context->device, fences[i], NULL);
        vkDestroySemaphore(context->device, acrquireSemaphores[i], NULL);
        vkDestroySemaphore(context->device, releaseSemaphores[i], NULL);

    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(context->device, commandPools[i], NULL);
    }

    destroyPipeline(context, &pipeline);

    vkDestroySampler(context->device, sampler, NULL);

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



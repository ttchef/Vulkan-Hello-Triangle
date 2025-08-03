
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "../include/vulkan_base.h"

void uploadDataToBuffer(VulkanContext *context, VulkanBuffer *buffer, void *data, size_t size) {
#if 0
    void* mapped;  
    if (vkMapMemory(context->device, buffer->memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        fprintf(stderr, "Failed to map memory for vulkan buffer!\n");
        return;
    }
    memcpy(mapped, data, size);
    vkUnmapMemory(context->device, buffer->memory);

#else 
    VulkanQueue* queue = &context->graphicsQueue;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VulkanBuffer stagingBuffer;

    createBuffer(context, &stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* mapped;
    if (vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        fprintf(stderr, "Failed to map memory for vulkan buffer!\n");
        return;
    }
    memcpy(mapped, data, size);
    vkUnmapMemory(context->device, stagingBuffer.memory);

    {
        VkCommandPoolCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queue->familyIndex;
        if (vkCreateCommandPool(context->device, &createInfo, NULL, &commandPool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create commandPool for staging buffer!\n");
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
            fprintf(stderr, "Failed to allocate command buffer for staging buffer!\n");
            return;
        }
    }

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to beging command buffer for staging buffer!\n");
        return;
    }

    VkBufferCopy region = (VkBufferCopy){ 0, 0, size };
    vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer->buffer, 1, &region);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to end command buffer for staging buffer!\n");
        return;
    }

    VkSubmitInfo subInfo = {0};
    subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    subInfo.commandBufferCount = 1;
    subInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue->queue, 1, &subInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit queue for staging buffer!\n");
        return;
    }

    if (vkQueueWaitIdle(queue->queue) != VK_SUCCESS) {
        fprintf(stderr, "Failed to wait for queue in staging buffer!\n");
        return;
    }

    vkDestroyCommandPool(context->device, commandPool, NULL);
    destroyBuffer(context, &stagingBuffer);

#endif
}

uint32_t findMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties) {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &deviceMemoryProperties);

    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        // Check if required memory type is allowed
        if ((typeFilter & (1 << i)) != 0) {
            // Check if the required properties are satisfied
            if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {
                return i;
            }
        }
    }

    // no matching type found
    fprintf(stderr, "Failed to find matching memory type!\n");
    return UINT32_MAX;
}

void createBuffer(VulkanContext *context, VulkanBuffer *buffer, uint64_t size,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {
    
    VkBufferCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;

    if (vkCreateBuffer(context->device, &createInfo, NULL, &buffer->buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create buffer!\n");
        return;
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(context->device, buffer->buffer, &memoryRequirements);

    uint32_t memoryIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, memoryProperties);
    if (memoryIndex == UINT32_MAX) {
        return;
    }

    VkMemoryAllocateInfo alloInfo = {0};
    alloInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloInfo.allocationSize = memoryRequirements.size;
    alloInfo.memoryTypeIndex = memoryIndex;

    if (vkAllocateMemory(context->device, &alloInfo, NULL, &buffer->memory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate vulkan memory for buffer!\n");
        return;
    }

    if (vkBindBufferMemory(context->device, buffer->buffer, buffer->memory, 0) != VK_SUCCESS) {
        fprintf(stderr, "Failed to bind buffer memory!\n");
        return;
    }


}

void destroyBuffer(VulkanContext *context, VulkanBuffer *buffer) {
    vkDestroyBuffer(context->device, buffer->buffer, NULL);

    // Assumes that the buffer owns its own memory block
    vkFreeMemory(context->device, buffer->memory, NULL);
}

void createImage(VulkanContext *context, VulkanImage *image, uint32_t width, uint32_t height,
                 VkFormat format, VkImageUsageFlags usage) {
    {
        VkImageCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.extent.width = width;
        createInfo.extent.height = height;
        createInfo.extent.depth = 1;
        createInfo.mipLevels = 1;
        createInfo.arrayLayers = 1;
        createInfo.format = format;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.usage = usage;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(context->device, &createInfo, NULL, &image->image) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create vulkan image!\n");
            exit(-1);
        }
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(context->device, image->image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(context->device, &allocInfo, NULL, &image->memory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory for image!\n");
        exit(-1);
    }

    if (vkBindImageMemory(context->device, image->image, image->memory, 0) != VK_SUCCESS) {
        fprintf(stderr, "Failed to bind memory!\n");
        exit(-1);
    }

    {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image->image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context->device, &createInfo, NULL, &image->view) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image view!\n");
            exit(-1);
        }
    }

}

void destroyImage(VulkanContext *context, VulkanImage *image) {
    vkDestroyImageView(context->device, image->view, NULL);
    vkDestroyImage(context->device, image->image, NULL);
    vkFreeMemory(context->device, image->memory, NULL);
}


void uploadDataToImage(VulkanContext *context, VulkanImage *image, void* data,
                       uint32_t size, uint32_t width, uint32_t height,
                       VkImageLayout finalLayout, VkAccessFlags dstAccessMask) {
    VulkanQueue* queue = &context->graphicsQueue;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VulkanBuffer stagingBuffer;

    createBuffer(context, &stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* mapped;
    if (vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        fprintf(stderr, "Failed to map memory for vulkan buffer!\n");
        return;
    }
    memcpy(mapped, data, size);
    vkUnmapMemory(context->device, stagingBuffer.memory);

    {
        VkCommandPoolCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = queue->familyIndex;
        if (vkCreateCommandPool(context->device, &createInfo, NULL, &commandPool) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create commandPool for staging buffer!\n");
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
            fprintf(stderr, "Failed to allocate command buffer for staging buffer!\n");
            return;
        }
    }

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to beging command buffer for staging buffer!\n");
        return;
    }

    {
        VkImageMemoryBarrier imageBarrier = {0};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = image->image;
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
            0, 0, 0, 0, 0, 1, &imageBarrier
        );
    }

    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = (VkExtent3D){ width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    {
        VkImageMemoryBarrier imageBarrier = {0};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = finalLayout;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = image->image;
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, 0, 0, 0, 1, &imageBarrier
        );
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to end command buffer for staging buffer!\n");
        exit(-1);
    }

    VkSubmitInfo subInfo = {0};
    subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    subInfo.commandBufferCount = 1;
    subInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue->queue, 1, &subInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit queue for staging buffer!\n");
        exit(-1);
    }

    if (vkQueueWaitIdle(queue->queue) != VK_SUCCESS) {
        fprintf(stderr, "Failed to wait for queue in staging buffer!\n");
        exit(-1);
    }

    vkDestroyCommandPool(context->device, commandPool, NULL);
    destroyBuffer(context, &stagingBuffer);

}


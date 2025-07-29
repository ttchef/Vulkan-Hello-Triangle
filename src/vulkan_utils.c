
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


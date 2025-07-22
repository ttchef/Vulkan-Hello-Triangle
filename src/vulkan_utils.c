
#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "../include/vulkan_base.h"

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


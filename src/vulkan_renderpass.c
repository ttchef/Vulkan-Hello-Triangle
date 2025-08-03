
#include <stdio.h> 
#include <vulkan/vulkan_core.h>

#include "../include/vulkan_base.h"

VkRenderPass createRenderPass(VulkanContext *context, VkFormat format) {
    VkRenderPass renderPass;
    
    VkAttachmentDescription attachmentDescriptions[2] = {0};
    attachmentDescriptions[0].format = format;
    attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT; // 1:1 pixel samples
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachmentDescriptions[1].format = VK_FORMAT_D32_SFLOAT;
    attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT; // 1:1 pixel samples
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference attachmentReference = {0};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthStencilReference = {0};
    depthStencilReference.attachment = 1;
    depthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;
    subpass.pDepthStencilAttachment = &depthStencilReference;

    VkRenderPassCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = ARRAY_COUNT(attachmentDescriptions);
    createInfo.pAttachments = attachmentDescriptions;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(context->device, &createInfo, NULL, &renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vulkan renderpass!\n");
        return renderPass;
    }
    
    return renderPass;
}

void destroyRenderPass(VulkanContext *context, VkRenderPass renderPass) {
    vkDestroyRenderPass(context->device, renderPass, NULL);
}



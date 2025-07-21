
#include <stdio.h> 
#include <vulkan/vulkan_core.h>

#include "../include/vulkan_base.h"

VkRenderPass createRenderPass(VulkanContext *context, VkFormat format) {
    VkRenderPass renderPass;
    
    VkAttachmentDescription attachmentDescription = {0};
    attachmentDescription.format = format;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT; // 1:1 pixel samples
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachmentReference = {0};
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachmentReference;

    VkRenderPassCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &attachmentDescription;
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



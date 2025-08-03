
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <vulkan/vulkan_core.h>

#include "../include/vulkan_base.h"


VkShaderModule createShaderModule(VulkanContext *context, const char *filepath) {
    VkShaderModule result = {0};

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Cant open Shader file: %s\n", filepath);
        return result;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // works because one spv command is 4 bytes so every file should be a multiple of 4
    // if not we know something isnt right here
    if ((fileSize & 0x03) != 0) {
        fprintf(stderr, "Error fileSize of shader file: %s isnt a multiple of 4!\n", filepath);
        return result;
    }

    uint8_t* buffer = malloc(sizeof(uint8_t) * fileSize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for shader: %s buffer!\n", filepath);
        return result;
    }
    fread(buffer, 1, fileSize, file);

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = (uint32_t*)buffer;
    
    if (vkCreateShaderModule(context->device, &createInfo, NULL, &result) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shade module: %s!\n", filepath);
        return result;
    }

    free(buffer);
    fclose(file);

    return result;
}

VulkanPipeline createPipeline(VulkanContext *context, const char *vertPath, const char *fragPath,
        VkRenderPass renderPass, uint32_t width, uint32_t height,
        VkVertexInputAttributeDescription* attributes, uint32_t numAttributes,
        VkVertexInputBindingDescription* binding, uint32_t numSetLayouts,
        VkDescriptorSetLayout* setLayouts, VkPushConstantRange* pushConstant) {

    VkShaderModule vertexShaderModule = createShaderModule(context, vertPath);
    VkShaderModule fragmentShaderModule = createShaderModule(context, fragPath);

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = (VkPipelineShaderStageCreateInfo){0};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShaderModule;
    shaderStages[0].pName = "main";

    shaderStages[1] = (VkPipelineShaderStageCreateInfo){0};
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShaderModule;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputState = {0};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputState.vertexAttributeDescriptionCount = numAttributes;
    vertexInputState.pVertexAttributeDescriptions = attributes;
    vertexInputState.vertexBindingDescriptionCount = binding ? 1 : 0;
    vertexInputState.pVertexBindingDescriptions = binding;

    VkPipelineInputAssemblyStateCreateInfo inpuAssemblyState = {0};
    inpuAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inpuAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   
    // Depricated because of dynamicStates
    //VkViewport viewport = (VkViewport){0.0f, 0.0f, (float)width, (float)(height)};
    //VkRect2D scissor = (VkRect2D){{0, 0}, {width, height}};
    
    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {0};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = {0};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStancilState = {0};
    depthStancilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStancilState.depthTestEnable = VK_TRUE;
    depthStancilState.depthWriteEnable = VK_TRUE;
    depthStancilState.depthCompareOp = VK_COMPARE_OP_GREATER;
    depthStancilState.maxDepthBounds = 0.0f;
    depthStancilState.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {0};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;

    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    dynamicState.dynamicStateCount = ARRAY_COUNT(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayout pipelineLayout;

    {
        VkPipelineLayoutCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.setLayoutCount = numSetLayouts;
        createInfo.pSetLayouts = setLayouts;
        createInfo.pushConstantRangeCount = pushConstant ? 1 : 0;
        createInfo.pPushConstantRanges = pushConstant;

        if (vkCreatePipelineLayout(context->device, &createInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create pipelineLayout!\n");
            exit(-1);
        }
    }

    VkPipeline pipeline;

    {
        VkGraphicsPipelineCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.stageCount = ARRAY_COUNT(shaderStages);
        createInfo.pStages = shaderStages;
        createInfo.pVertexInputState = &vertexInputState;
        createInfo.pInputAssemblyState = &inpuAssemblyState;
        createInfo.pViewportState = &viewportState;
        createInfo.pRasterizationState = &rasterizationState;
        createInfo.pMultisampleState = &multisampleState;
        createInfo.pDepthStencilState = &depthStancilState;
        createInfo.pColorBlendState = &colorBlendState;
        createInfo.pDynamicState = &dynamicState;
        createInfo.layout = pipelineLayout;
        createInfo.renderPass = renderPass;
        createInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(context->device, 0, 1, &createInfo, 0, &pipeline) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create Graphics pipeline!\n");
            exit(-1);
        }
    }

    // module can be destroyed after pipeline creation 
    vkDestroyShaderModule(context->device, vertexShaderModule, NULL);
    vkDestroyShaderModule(context->device, fragmentShaderModule, NULL);

    VulkanPipeline result = {0};
    result.pipeline = pipeline;
    result.layout = pipelineLayout;

    return result;

}

void destroyPipeline(VulkanContext *context, VulkanPipeline *pipeline) {
    vkDestroyPipeline(context->device, pipeline->pipeline, NULL);
    vkDestroyPipelineLayout(context->device, pipeline->layout, NULL);
}


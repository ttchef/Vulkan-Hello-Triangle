
#ifndef MODEL_H
#define MODEL_H

#include "vulkan_base.h"

typedef struct {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    uint64_t numIndices;
    VulkanImage albedoTexture; // color texture
} Model;

Model createModel(VulkanContext* context, const char* filepath);
void destroyModel(VulkanContext* context, Model* model);

#endif


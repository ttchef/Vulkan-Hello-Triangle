
#include "../include/model.h"
#include <assert.h>
#include <vulkan/vulkan_core.h>

#define CGLTF_IMPLEMENTATION
#include "../vendor/cgltf/cgltf.h"

// stride in bytes and element size in bytes
void fillBuffer(uint32_t inputStride, void* inputData, uint32_t outputStride, void* outputData,
                uint32_t numElements, uint32_t elementSize) {
    uint8_t* output = (uint8_t*)outputData;
    uint8_t* input = (uint8_t*)inputData;
    for (uint32_t i = 0; i < numElements; i++) {
        for (uint32_t b = 0; b < elementSize; b++) {
            output[b] = input[b];
        }
        output += outputStride;
        input += inputStride;
    }
}

Model createModel(VulkanContext *context, const char *filepath) {
    Model result = {0};

    cgltf_options options = {0};
    cgltf_data* data = NULL;

    cgltf_result error = cgltf_parse_file(&options, filepath, &data);
    if (error == cgltf_result_success) {
        error = cgltf_load_buffers(&options, data, "/home/ttchef/coding/c/Vulkan-Hello-Triangle/res/models");
        if (error == cgltf_result_success) {
            assert(data->meshes_count == 1);
            assert(data->meshes[0].primitives_count == 1);
            assert(data->meshes[0].primitives[0].attributes_count > 0);
            assert(data->meshes[0].primitives[0].attributes[0].type == cgltf_attribute_type_position);
            assert(data->meshes[0].primitives[0].indices->component_type == cgltf_component_type_r_16u);
            assert(data->meshes[0].primitives[0].indices->stride == sizeof(uint16_t));
            
            // indices
            uint8_t* bufferBase = (uint8_t*)data->meshes[0].primitives[0].indices->buffer_view->buffer->data;
            uint64_t indexDataSize = data->meshes[0].primitives[0].indices->buffer_view->size;
            void* indexData = bufferBase + data->meshes[0].primitives[0].indices->buffer_view->offset;

            createBuffer(context, &result.indexBuffer, indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            uploadDataToBuffer(context, &result.indexBuffer, indexData, indexDataSize);
            result.numIndices = data->meshes[0].primitives[0].indices->count;

            // Vertices
            uint64_t numVertices = data->meshes[0].primitives[0].attributes->data->count;
            uint64_t vertexDataSize = sizeof(float) * 6 * numVertices; // pos and normal
            uint8_t* vertexData = malloc(sizeof(uint8_t) * vertexDataSize);

            for (uint64_t i = 0; i < data->meshes[0].primitives[0].attributes_count; i++) {
                cgltf_attribute* attribute = data->meshes[0].primitives[0].attributes + i; // not [i] because of ptr
                if (attribute->type == cgltf_attribute_type_position) {
                    bufferBase = (uint8_t*)attribute->data->buffer_view->buffer->data;
                    uint64_t positionDataSize = attribute->data->buffer_view->size;
                    void* positionData = bufferBase + attribute->data->buffer_view->offset;
                    fillBuffer(sizeof(float) * 3, positionData, sizeof(float) * 6, vertexData, numVertices, sizeof(float) * 3);
                }
                else if (attribute->type == cgltf_attribute_type_normal) {
                    assert(data->meshes[0].primitives[0].attributes[0].data->stride == sizeof(float) * 3);
                    bufferBase = (uint8_t*)attribute->data->buffer_view->buffer->data;
                    uint64_t normalDataSize = attribute->data->buffer_view->size;
                    void* normalData = bufferBase + attribute->data->buffer_view->offset;
                    fillBuffer(sizeof(float) * 3, normalData, sizeof(float) * 6, vertexData + (sizeof(float) * 3), numVertices, sizeof(float) * 3);
                }
            }

            createBuffer(context, &result.vertexBuffer, vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            uploadDataToBuffer(context, &result.vertexBuffer, vertexData, vertexDataSize);

        }
        
        cgltf_free(data);
    }

    return result;
}

void destroyModel(VulkanContext *context, Model *model) {
    destroyBuffer(context, &model->vertexBuffer);
    destroyBuffer(context, &model->indexBuffer);
    *model = (Model){0};
}


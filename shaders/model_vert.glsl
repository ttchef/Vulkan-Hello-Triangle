#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout (set = 0, binding = 0) uniform transforms {
    mat4 modelViewProj;
} u_transforms;

layout (location = 0) out vec3 out_normal;

void main() {
    mat4 modelViewProj = u_transforms.modelViewProj;
    gl_Position = modelViewProj * vec4(in_pos.x, in_pos.y, in_pos.z, 1.0);
    out_normal = in_normal;
}


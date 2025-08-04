#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout (push_constant) uniform pushConstans {
    mat4 modelViewProj;
} u_push_constants;

layout (location = 0) out vec3 out_normal;

void main() {
    mat4 modelViewProj = u_push_constants.modelViewProj;
    gl_Position = modelViewProj * vec4(in_pos.x, in_pos.y, in_pos.z, 1.0);
    out_normal = in_normal;
}


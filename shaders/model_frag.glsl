

#version 450 core

layout (location = 0) in vec3 in_normal;
layout (location = 1) in vec2 in_texcoord;

layout (set = 0, binding = 1) uniform sampler2D in_albedoSampledTexture;

layout (location = 0) out vec4 out_color;

void main() {
    vec4 texSample = texture(in_albedoSampledTexture, in_texcoord);

    vec3 normal = normalize(in_normal);
    vec3 lightDir = normalize(vec3(0.0, 1.0, 0.0));

    float brightness = max(dot(lightDir, normal), 0.0);

    //out_color = vec4(normal * 0.5 + 0.5, 1.0);
    //out_color = texSample;
    out_color = vec4(texSample.x * brightness, texSample.y * brightness, texSample.z * brightness, texSample.w);
}





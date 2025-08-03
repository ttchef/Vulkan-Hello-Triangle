#!/bin/bash

glslc -fshader-stage=vert shaders/triangle_vert.glsl -o shaders/triangle_vert.spv
glslc -fshader-stage=frag shaders/triangle_frag.glsl -o shaders/triangle_frag.spv

glslc -fshader-stage=vert shaders/color_vert.glsl -o shaders/color_vert.spv
glslc -fshader-stage=frag shaders/color_frag.glsl -o shaders/color_frag.spv

glslc -fshader-stage=vert shaders/texture_vert.glsl -o shaders/texture_vert.spv
glslc -fshader-stage=frag shaders/texture_frag.glsl -o shaders/texture_frag.spv

glslc -fshader-stage=vert shaders/model_vert.glsl -o shaders/model_vert.spv
glslc -fshader-stage=frag shaders/model_frag.glsl -o shaders/model_frag.spv


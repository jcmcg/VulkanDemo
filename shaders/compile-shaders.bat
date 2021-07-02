@echo off
echo Compiling shaders...
echo "glsl/shader.vert.glsl => spv/shader.vert.spv"
glslc.exe -fshader-stage=vert glsl/shader.vert.glsl -o spv/shader.vert.spv
echo "glsl/shader.frag.glsl => spv/shader.frag.spv"
glslc.exe -fshader-stage=frag glsl/shader.frag.glsl -o spv/shader.frag.spv
echo Done!

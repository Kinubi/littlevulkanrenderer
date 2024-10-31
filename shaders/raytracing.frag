#version 450

// Fragment shader to read from a texture
layout(set=0, binding = 0, rgba32f) readonly uniform image2D imgInput;
layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = imageLoad(imgInput, ivec2(gl_FragCoord.xy));
}

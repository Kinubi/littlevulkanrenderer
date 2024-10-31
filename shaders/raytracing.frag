#version 450

// Fragment shader to read from a texture
layout(set=0, binding = 0) uniform sampler2D imgInput;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 fragColor;

void main() {
   fragColor = texture(imgInput, fragTexCoord);
}

#include "texture.h"

#include <stdexcept>

namespace lvr {

Texture::Texture(const std::string& filePath) : filePath(filePath) { createTextureImage(); }

void Texture::createTextureImage() {
	pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
}
// void Texture::createImage(
// 	Device& device,
// 	uint32_t width,
// 	uint32_t height,
// 	VkFormat format,
// 	VkImageTiling tiling,
// 	VkImageUsageFlags usage,
// 	VkMemoryPropertyFlags properties,
// 	VkImage& image,
// 	VkDeviceMemory& imageMemory) {
// 	Buffer stagingBuffer{
// 		device,
// 		imageSize,
// 		(uint32_t)1,
// 		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
// 		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
// 		1};
// 	VkMemoryRequirements memRequirements;
// 	vkGetImageMemoryRequirements(device.device(), textureImage, &memRequirements);

// 	stagingBuffer.map();
// 	stagingBuffer.writeToBuffer((void*)pixels);
// 	textureBuffer = std::make_unique<Buffer>(device.device(), imageSize, 1, usage);
// 	device.copyBuffer(stagingBuffer.getBuffer(), textureBuffer->getBuffer(), memRequirements.size);
// }

}  // namespace lvr
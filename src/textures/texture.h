#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <memory>

#include "buffer.h"
#include "device.h"
#include "vulkan/vulkan.h"
namespace lvr {

class Texture {
   public:
	Texture(const std::string& filePath);
	~Texture() {};

	void createTextureImage();

	// void createImage(
	// 	Device& device,
	// 	uint32_t width,
	// 	uint32_t height,
	// 	VkFormat format,
	// 	VkImageTiling tiling,
	// 	VkImageUsageFlags usage,
	// 	VkMemoryPropertyFlags properties,
	// 	VkImage& image,
	// 	VkDeviceMemory& imageMemory);

   private:
	// Device& device;
	const std::string& filePath;
	int32_t texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels;
	VkDeviceSize imageSize;

	VkImage textureImage;
	std::unique_ptr<Buffer> textureBuffer;
};
}  // namespace lvr
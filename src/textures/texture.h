#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "buffer.h"
#include "device.h"
namespace lvr {

	class Texture {
	public:
		Texture(Device& device, const std::string& filePath);
		~Texture() {};

		void createTextureImage();

		void createImage(
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties);



	private:
		Device& device;
		const std::string& filePath;
		int32_t texWidth = 0, texHeight = 0, texChannels = 0;
		stbi_uc* pixels;
		VkDeviceSize imageSize;

		std::unique_ptr<Buffer> stagingBuffer;

		VkImage textureImage;
		VkDeviceMemory imageMemory;
	};
}  // namespace lvr
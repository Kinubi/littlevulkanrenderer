#pragma once

#include "device.h"

// libs
#include <vulkan/vulkan.h>

// std
#include <memory>
#include <string>

namespace lvr {
class Texture {
   public:
	Texture(Device &device, const std::string &textureFilepath);
	Texture(
		Device &device,
		VkFormat format,
		VkExtent3D extent,
		VkImageUsageFlags usage,
		VkSampleCountFlagBits sampleCount);
	~Texture();

	// delete copy constructors
	Texture(const Texture &) = delete;
	Texture &operator=(const Texture &) = delete;

	VkImageView imageView() const { return mTextureImageView; }
	VkSampler sampler() const { return mTextureSampler; }
	VkImage getImage() const { return mTextureImage; }
	VkImageView getImageView() const { return mTextureImageView; }
	VkDescriptorImageInfo getImageInfo() const { return mDescriptor; }
	VkImageLayout getImageLayout() const { return mTextureLayout; }
	VkExtent3D getExtent() const { return mExtent; }
	VkFormat getFormat() const { return mFormat; }

	void updateDescriptor();
	void transitionLayout(
		VkCommandBuffer commandBuffer,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		bool before);

	static std::unique_ptr<Texture> createTextureFromFile(
		Device &device, const std::string &filepath);

   private:
	void createTextureImage(const std::string &filepath);
	void createTextureImageView(VkImageViewType viewType);
	void createTextureSampler();

	VkDescriptorImageInfo mDescriptor{};

	Device &mDevice;
	VkImage mTextureImage = nullptr;
	VkDeviceMemory mTextureImageMemory = nullptr;
	VkImageView mTextureImageView = nullptr;
	VkSampler mTextureSampler = nullptr;
	VkFormat mFormat;
	VkImageLayout mTextureLayout;
	uint32_t mMipLevels{1};
	uint32_t mLayerCount{1};
	VkExtent3D mExtent{};
};

}  // namespace lvr
#pragma once

#include <memory>

#include "buffer.h"
#include "descriptors.h"
#include "frameinfo.h"
#include "pipeline.h"
#include "swapchain.h"

namespace lvr {

class ComputeShader {
   public:
	const uint32_t MAX_GROUPS_X = 32;
	ComputeShader(
		Device &device,
		VkRenderPass renderPass,
		std::vector<std::string> filePaths,
		std::unique_ptr<DescriptorSetLayout> externalSetLayout);
	~ComputeShader();

	ComputeShader(const ComputeShader &) = delete;
	ComputeShader &operator=(const ComputeShader &) = delete;

	void dispatchComputeShader(
		std::vector<std::unique_ptr<Buffer>> &ubos,
		FrameInfo frameInfo,
		VkCommandBuffer computeCommandBuffer,
		VkDescriptorImageInfo imageInfo,
		glm::vec2 workGroupCount = glm::vec2(1, 1));

	std::vector<std::unique_ptr<Buffer>> shaderStorageBuffers;

	template <typename T>
	void createShaderStorageBuffers(const std::vector<T> computeBufferData);
	template <typename T>
	std::vector<T> getShaderStorageBuffers();

   private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);

	Device &device;

	uint32_t bufferCount;

	std::unique_ptr<Pipeline> computePipeline;
	VkPipelineLayout computePipelineLayout{};
	std::vector<std::string> filePaths;

	bool isComputeDispatched;

	int32_t currentFrameIndex{0};
	std::unique_ptr<DescriptorSetLayout> computeShaderLayout{};
	std::unique_ptr<DescriptorSetLayout> externalSetLayout{};
};

template <typename T>
void ComputeShader::createShaderStorageBuffers(const std::vector<T> computeBufferData) {
	// Create a staging buffer used to upload data to the gpu
	bufferCount = static_cast<uint32_t>(computeBufferData.size());
	uint32_t bufferItemSize = sizeof(computeBufferData[0]);
	VkDeviceSize bufferSize = bufferItemSize * bufferCount;

	Buffer stagingBuffer{
		device,
		bufferItemSize,
		bufferCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void *)computeBufferData.data());

	shaderStorageBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	// Copy initial particle data to all storage buffers
	for (size_t i = 0; i < SwapChain::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		shaderStorageBuffers[i] = std::make_unique<Buffer>(
			device,
			bufferItemSize,
			bufferCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		;
		device.copyBuffer(
			stagingBuffer.getBuffer(),
			shaderStorageBuffers[i]->getBuffer(),
			bufferSize);
	}
}

template <typename T>
std::vector<T> ComputeShader::getShaderStorageBuffers() {
	// Purely for debuggging compute shader
	Buffer stagingBuffer{
		device,
		sizeof(T),
		bufferCount,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};
	device.copyBuffer(
		shaderStorageBuffers[currentFrameIndex]->getBuffer(),
		stagingBuffer.getBuffer(),
		(VkDeviceSize)(bufferCount * sizeof(T)));

	stagingBuffer.map();
	std::vector<T> bufferData;
	bufferData.reserve(bufferCount * sizeof(T));
	std::vector<T> *ssboBlockPointer = (std::vector<T> *)stagingBuffer.getMappedMemory();
	memcpy(&bufferData, ssboBlockPointer, sizeof(bufferData));
	// std::cout << particle.position.x << std::endl;
}
}  // namespace lvr

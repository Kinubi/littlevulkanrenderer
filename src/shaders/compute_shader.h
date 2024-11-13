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
		std::unique_ptr<DescriptorSetLayout> computeShaderLayout);
	~ComputeShader();

	ComputeShader(const ComputeShader &) = delete;
	ComputeShader &operator=(const ComputeShader &) = delete;

	void dispatchComputeShader(
		VkCommandBuffer computeCommandBuffer,
		VkDescriptorSet computeDescriptorSet,
		glm::vec2 workGroupCount = glm::vec2(1, 1));

	template <typename T>
	std::vector<std::unique_ptr<Buffer>> createShaderStorageBuffers(
		const std::vector<T> computeBufferData);
	template <typename T>
	void getShaderStorageBuffer(std::unique_ptr<Buffer> &shaderBuffer, std::vector<T> &bufferData);

	template <typename T>
	std::unique_ptr<Buffer> createShaderStorageBuffer(const std::vector<T> computeBufferData);

	std::unique_ptr<DescriptorSetLayout> &getComputeShaderLayout() { return computeShaderLayout; }

   private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);

	Device &device;

	std::unique_ptr<Pipeline> computePipeline;
	VkPipelineLayout computePipelineLayout{};
	std::vector<std::string> filePaths;

	bool isComputeDispatched;

	int32_t currentFrameIndex{0};
	std::unique_ptr<DescriptorSetLayout> computeShaderLayout{};
};

template <typename T>
std::vector<std::unique_ptr<Buffer>> ComputeShader::createShaderStorageBuffers(
	const std::vector<T> computeBufferData) {
	// Create a staging buffer used to upload data to the gpu
	uint32_t bufferCount = static_cast<uint32_t>(computeBufferData.size());
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
	std::vector<std::unique_ptr<Buffer>> storageBuffers;

	storageBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	// Copy initial particle data to all storage buffers
	for (size_t i = 0; i < SwapChain::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		storageBuffers[i] = std::make_unique<Buffer>(
			device,
			bufferItemSize,
			bufferCount,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		;
		device.copyBuffer(stagingBuffer.getBuffer(), storageBuffers[i]->getBuffer(), bufferSize);
	}

	return storageBuffers;
}

template <typename T>
void ComputeShader::getShaderStorageBuffer(
	std::unique_ptr<Buffer> &shaderBuffer, std::vector<T> &bufferData) {
	// Purely for debuggging compute shader
	uint32_t bufferCount = static_cast<uint32_t>(bufferData.size());
	uint32_t bufferItemSize = sizeof(bufferData[0]);
	VkDeviceSize bufferSize = bufferItemSize * bufferCount;
	Buffer stagingBuffer{
		device,
		sizeof(T),
		bufferCount,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};
	device.copyBuffer(
		shaderBuffer->getBuffer(),
		stagingBuffer.getBuffer(),
		(VkDeviceSize)(bufferCount * sizeof(T)));

	stagingBuffer.map();
	bufferData.reserve(bufferCount * sizeof(T));
	std::vector<T> *ssboBlockPointer = (std::vector<T> *)stagingBuffer.getMappedMemory();
	memcpy(&bufferData, ssboBlockPointer, sizeof(bufferData));
	// std::cout << particle.position.x << std::endl;
}

template <typename T>
std::unique_ptr<Buffer> ComputeShader::createShaderStorageBuffer(
	const std::vector<T> computeBufferData) {
	// Create a staging buffer used to upload data to the gpu
	uint32_t bufferCount = static_cast<uint32_t>(computeBufferData.size());
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
	std::unique_ptr<Buffer> storageBuffer;

	// Copy initial particle data to all storage buffers

	storageBuffer = std::make_unique<Buffer>(
		device,
		bufferItemSize,
		bufferCount,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	device.copyBuffer(stagingBuffer.getBuffer(), storageBuffer->getBuffer(), bufferSize);

	storageBuffer->map();

	return storageBuffer;
}

}  // namespace lvr

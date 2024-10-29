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
	const uint32_t MAX_GROUPSX = 1024;
	ComputeShader(Device &device, VkRenderPass renderPass, std::vector<std::string> filePaths);
	~ComputeShader();

	ComputeShader(const ComputeShader &) = delete;
	ComputeShader &operator=(const ComputeShader &) = delete;

	void dispatchComputeShader(std::vector<std::unique_ptr<Buffer>> &ubos, FrameInfo frameInfo);
	VkCommandBuffer beginCompute();
	void endCompute();
	VkCommandBuffer getCurrentComputeCommandBuffer() const {
		assert(isComputeDispatched && "Cannot get command buffer when frame not in progress!");
		return computeCommandBuffers[currentFrameIndex];
	}
	int32_t getFrameIndex() const {
		assert(isComputeDispatched && "Cannot get frame index when frame is not in progress");
		return currentFrameIndex;
	}

	std::vector<std::unique_ptr<Buffer>> shaderStorageBuffers;
	std::vector<VkCommandBuffer> computeCommandBuffers;

	template <typename T>
	void createShaderStorageBuffers(const std::vector<T> computeBufferData);

   private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
	void createComputeCommandBuffers();
	void freeComputeCommandBuffers();

	Device &device;

	uint32_t bufferCount;

	std::unique_ptr<Pipeline> computePipeline;
	VkPipelineLayout computePipelineLayout{};
	std::vector<std::string> filePaths;

	bool isComputeDispatched;

	int32_t currentFrameIndex{0};
	std::unique_ptr<DescriptorSetLayout> computeShaderLayout{};
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
}  // namespace lvr

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

	void dispatchComputeShader(
		VkDescriptorSet &uniformDiscriptorSet, std::vector<Buffer> ubos, FrameInfo frameInfo);
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

   private:
	template <typename T>
	void createShaderStorageBuffers(const std::vector<T> computeBufferData);
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
	void createComputeCommandBuffers();
	void freeComputeCommandBuffers();

	Device &device;

	std::vector<std::unique_ptr<Buffer>> shaderStorageBuffers;
	uint32_t bufferCount;

	std::unique_ptr<Pipeline> computePipeline;
	VkPipelineLayout computePipelineLayout{};
	std::vector<std::string> filePaths;

	bool isComputeDispatched;
	std::vector<VkCommandBuffer> computeCommandBuffers;

	int32_t currentFrameIndex{0};
	std::unique_ptr<DescriptorSetLayout> computeShaderLayout{};
};
}  // namespace lvr

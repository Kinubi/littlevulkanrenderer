#pragma once

#include <memory>

#include "buffer.h"
#include "descriptors.h"
#include "pipeline.h"
#include "swapchain.h"

namespace lvr {
template <typename T>
class ComputeShader {
   public:
	ComputeShader(
		Device &device,
		VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout,
		const std::vector<T> computeBufferData,
		std::vector<std::string> filePaths);
	~ComputeShader() {};

	ComputeShader(const ComputeShader &) = delete;
	ComputeShader &operator=(const ComputeShader &) = delete;

   private:
	void createShaderStorageBuffers(const std::vector<T> computeBufferData);
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	void dispatchComputeShader(VkCommandBuffer commandBuffer);

	Device &device;

	std::vector<std::unique_ptr<Buffer>> shaderStorageBuffers;
	uint32_t bufferCount;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout{};
	std::vector<std::string> filePaths;

	std::unique_ptr<DescriptorSetLayout> computeShaderLayout{};
};
}  // namespace lvr
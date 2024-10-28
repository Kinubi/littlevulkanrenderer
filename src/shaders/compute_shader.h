#pragma once

#include <memory>

#include "buffer.h"
#include "descriptors.h"
#include "pipeline.h"
#include "swapchain.h"

namespace lvr {
class ComputeShader {
   public:
	ComputeShader(
		Device &device,
		VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout,
		VkDeviceSize bufferSize,
		std::vector<std::string> filePaths);
	~ComputeShader() {};

	ComputeShader(const ComputeShader &) = delete;
	ComputeShader &operator=(const ComputeShader &) = delete;

   private:
	void createShaderStorageBuffers(VkDeviceSize bufferSize);
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	void dispatchComputeShader(VkCommandBuffer commandBuffer);

	Device &device;

	std::vector<std::unique_ptr<Buffer>> shaderStorageBuffers;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout{};
	std::vector<std::string> filePaths;

	std::unique_ptr<DescriptorSetLayout> computeShaderLayout{};
};
}  // namespace lvr
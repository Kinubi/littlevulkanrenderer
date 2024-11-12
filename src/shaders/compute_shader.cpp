#include "compute_shader.h"

#include <iostream>

namespace lvr {

ComputeShader::ComputeShader(
	Device& device,
	VkRenderPass renderPass,
	std::vector<std::string> filePaths,
	std::unique_ptr<DescriptorSetLayout> computeShaderLayout)
	: device(device), filePaths(filePaths), computeShaderLayout(std::move(computeShaderLayout)) {
	createPipelineLayout();
	createPipeline(renderPass);
}

ComputeShader::~ComputeShader() {
	vkDestroyPipelineLayout(device.device(), computePipelineLayout, nullptr);
}

void ComputeShader::dispatchComputeShader(
	VkCommandBuffer computeCommandBuffer,
	VkDescriptorSet computeDescriptorSet,
	glm::vec2 workGroupCount) {
	computePipeline->bindCompute(computeCommandBuffer);

	vkCmdBindDescriptorSets(
		computeCommandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		computePipelineLayout,
		0,
		1,
		&computeDescriptorSet,
		0,
		nullptr);

	vkCmdDispatch(computeCommandBuffer, workGroupCount.x, workGroupCount.y, 1);
}

void ComputeShader::createPipelineLayout() {
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		computeShaderLayout->getDescriptorSetLayout()};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	if (vkCreatePipelineLayout(
			device.device(),
			&pipelineLayoutInfo,
			nullptr,
			&computePipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create computePipeline layout");
	}
}

void ComputeShader::createPipeline(VkRenderPass renderPass) {
	assert(
		computePipelineLayout != nullptr &&
		"Cannot create computePipeline before computePipeline layout");

	PipelineConfigInfo pipelineConfig{};
	pipelineConfig.pipelineLayout = computePipelineLayout;
	computePipeline = std::make_unique<Pipeline>(device, filePaths, pipelineConfig);
}

}  // namespace lvr

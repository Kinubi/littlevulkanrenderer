#include "compute_shader.h"

#include <iostream>

namespace lvr {

ComputeShader::ComputeShader(
	Device& device, VkRenderPass renderPass, std::vector<std::string> filePaths)
	: device(device), filePaths(filePaths) {
	createPipelineLayout();
	createPipeline(renderPass);
}

ComputeShader::~ComputeShader() {
	vkDestroyPipelineLayout(device.device(), computePipelineLayout, nullptr);
}

void ComputeShader::dispatchComputeShader(
	std::vector<std::unique_ptr<Buffer>>& ubos,
	FrameInfo frameInfo,
	VkCommandBuffer computeCommandBuffer) {
	uint32_t frameIndex = frameInfo.frameIndex;

	computePipeline->bindCompute(computeCommandBuffer);

	VkDescriptorSet computeDescriptorSet;
	auto bufferInfoubo = ubos[frameIndex]->descriptorInfo();
	auto bufferInfoLastFrame =
		shaderStorageBuffers[(frameIndex - 1) % SwapChain::MAX_FRAMES_IN_FLIGHT]->descriptorInfo();
	auto bufferInfoCurrentFrame = shaderStorageBuffers[frameIndex]->descriptorInfo();

	DescriptorWriter(*computeShaderLayout, frameInfo.frameDescriptorPool)
		.writeBuffer(0, &bufferInfoubo)
		.writeBuffer(1, &bufferInfoLastFrame)
		.writeBuffer(2, &bufferInfoCurrentFrame)
		.build(computeDescriptorSet);

	vkCmdBindDescriptorSets(
		computeCommandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		computePipelineLayout,
		0,
		1,
		&computeDescriptorSet,
		0,
		nullptr);

	vkCmdDispatch(computeCommandBuffer, 32, 1, 1);
}

void ComputeShader::createPipelineLayout() {
	computeShaderLayout =
		DescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.build();

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

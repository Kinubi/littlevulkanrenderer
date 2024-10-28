#include "compute_shader.h"

namespace lvr {
template <typename T>
ComputeShader<T>::ComputeShader(
	Device& device,
	VkRenderPass renderPass,
	VkDescriptorSetLayout globalSetLayout,
	const std::vector<T> computeBufferData,
	std::vector<std::string> filePaths)
	: device(device), filePaths(filePaths) {
	createPipelineLayout<T>(globalSetLayout);
	createPipeline<T>(renderPass);
	createShaderStorageBuffers<T>(computeBufferData);
}
template <typename T>
void ComputeShader<T>::createShaderStorageBuffers(const std::vector<T> computeBufferData) {
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
	stagingBuffer.writeToBuffer((void*)computeBufferData.data());

	shaderStorageBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	// Copy initial particle data to all storage buffers
	for (size_t i = 0; i < SwapChain::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		shaderStorageBuffers[i] = std::make_unique<Buffer>(
			device,
			bufferItemSize,
			bufferCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		;
	}
	device.copyBuffer(stagingBuffer.getBuffer(), computeBufferData->getBuffer(), bufferSize);
}
template <typename T>
void ComputeShader<T>::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
	computeShaderLayout =
		DescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.build();

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		globalSetLayout,
		computeShaderLayout->getDescriptorSetLayout()};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}
template <typename T>
void ComputeShader<T>::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig, device.getMsaaSamples());

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<Pipeline>(device, filePaths, pipelineConfig);
}
}  // namespace lvr

#include "compute_shader.h"

namespace lvr {

ComputeShader::ComputeShader(
	Device& device, VkRenderPass renderPass, std::vector<std::string> filePaths)
	: device(device), filePaths(filePaths) {
	createPipelineLayout();
	createPipeline(renderPass);
}

ComputeShader::~ComputeShader() { freeComputeCommandBuffers(); }

void ComputeShader::dispatchComputeShader(
	VkDescriptorSet& uniformDiscriptorSet, std::vector<Buffer> ubos, FrameInfo frameInfo) {
	uint32_t frameIndex = getFrameIndex();
	VkCommandBuffer computeCommandBuffer = getCurrentComputeCommandBuffer();
	computePipeline->bindCompute(computeCommandBuffer);

	VkDescriptorSet computeDescriptorSet;
	auto bufferInfoubo = ubos[frameIndex].descriptorInfo();
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

	vkCmdDispatch(computeCommandBuffer, MAX_GROUPSX, 1, 1);
}

VkCommandBuffer ComputeShader::beginCompute() {
	assert(!isComputeDispatched && "Can't call beginFrame while already in progress");

	isComputeDispatched = true;

	auto computeCommandBuffer = getCurrentComputeCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	return computeCommandBuffer;
}

void ComputeShader::endCompute() {
	assert(isComputeDispatched && "Can't call endFrame while frame is not in progress");
	auto computeCommandBuffer = getCurrentComputeCommandBuffer();
	if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	isComputeDispatched = false;
}

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
		device.copyBuffer(
			stagingBuffer.getBuffer(),
			shaderStorageBuffers[i]->getBuffer(),
			bufferSize);
	}
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
	pipelineLayoutInfo.pushConstantRangeCount = 1;

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

void ComputeShader::createComputeCommandBuffers() {
	computeCommandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, computeCommandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate compute command buffers!");
	}
}

void ComputeShader::freeComputeCommandBuffers() {
	vkFreeCommandBuffers(
		device.device(),
		device.getCommandPool(),
		static_cast<uint32_t>(computeCommandBuffers.size()),
		computeCommandBuffers.data());
	computeCommandBuffers.clear();
}
}  // namespace lvr

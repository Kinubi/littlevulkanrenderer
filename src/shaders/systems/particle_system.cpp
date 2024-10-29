#include "particle_system.h"

namespace lvr {
ParticleSystem::ParticleSystem(Device& device, VkRenderPass renderPass) : device(device) {
	computeShader = std::make_unique<ComputeShader>(device, renderPass, {"shaders/particles.comp"});
	createPipelineLayout() createPipeline(renderPass);
}

void ParticleSystem::renderParticles(FrameInfo& frameinfo) {
	pipeline->bind(frameInfo.commandBuffer);
	auto projectionView = frameInfo.camera.getProjection() * frameInfo.camera.getView();

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(
		frameInfo.commandBuffer,
		0,
		1,
		&shaderStorageBuffers[frameinfo.frameIndex],
		offsets);

	vkCmdDraw(frameinfo.commandBuffer, PARTICLE_COUNT, 1, 0, 0);
	computeShader->beginCompute()
		computeShader->dispatchComputeShader(uniformDiscriptorSet, UniformBufferObject, )
}

void ParticleSystem::createPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;

	if (vkCreatePipelineLayout(lvrDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void ParticleSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig, lvrDevice.getMsaaSamples());

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	lvrPipeline = std::make_unique<Pipeline>(
		lvrDevice,
		std::vector<std::string>{
			"shaders/particles.vert",
			"shaders/particles.frag",
		},
		pipelineConfig);
}
void ParticleSystem::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	for (size_t i = 0; i < SwapChain::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		uniformBuffers[i] = std::make_unique<Buffer>(
			device,
			bufferSize,
			1,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		;
		uniformBuffers.map()
	}
}
void ParticleSystem::updateUniformBuffers(FrameInfo frameInfo) {
	UniformBufferObject ubo{};
	ubo.deltaTime = frameInfo.frameTime * 2.0f;

	uniformBuffers[frameInfo.frameIndex]->writeToBuffer(ubo);
}
}  // namespace lvr
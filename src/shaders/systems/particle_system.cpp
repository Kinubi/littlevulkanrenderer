#include "particle_system.h"

#include <cstring>
#include <iostream>
#include <random>

namespace lvr {
ParticleSystem::ParticleSystem(Device& device, VkRenderPass renderPass) : device(device) {
	computeShader = std::make_unique<ComputeShader>(
		device,
		renderPass,
		std::vector<std::string>{"shaders/particles.comp"},
		DescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.build());
	createPipelineLayout();
	createPipeline(renderPass);
	createParticles();
	createUniformBuffers();
	particlesBuffers = computeShader->createShaderStorageBuffers<Particle>(particles);
}

ParticleSystem::~ParticleSystem() {
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void ParticleSystem::createPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void ParticleSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig, device.getMsaaSamples());
	pipelineConfig.attributeDescriptions = Particle::getAttributeDescriptions();
	pipelineConfig.bindingDescriptions = Particle::getBindingDescriptions();
	pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<Pipeline>(
		device,
		std::vector<std::string>{
			"shaders/particles.vert",
			"shaders/particles.frag",
		},
		pipelineConfig);
}

void ParticleSystem::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	uniformBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		uniformBuffers[i] = std::make_unique<Buffer>(
			device,
			bufferSize,
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		;
		uniformBuffers[i]->map();
	}
}

void ParticleSystem::dispatchCompute(FrameInfo& frameInfo, VkCommandBuffer computeCommandBuffer) {
	VkDescriptorSet computeDescriptorSet;
	auto bufferInfoubo = uniformBuffers[frameIndex]->descriptorInfo();
	auto bufferInfoLastFrame =
		particlesBuffers[(frameIndex - 1) % SwapChain::MAX_FRAMES_IN_FLIGHT]->descriptorInfo();
	auto bufferInfoCurrentFrame = particlesBuffers[frameIndex]->descriptorInfo();

	DescriptorWriter(*computeShader->getComputeShaderLayout(), frameInfo.frameDescriptorPool)
		.writeBuffer(0, &bufferInfoubo)
		.writeBuffer(1, &bufferInfoLastFrame)
		.writeBuffer(2, &bufferInfoCurrentFrame)
		.build(computeDescriptorSet);

	computeShader->dispatchComputeShader(
		computeCommandBuffer,
		computeDescriptorSet,
		glm::vec2(32, 1));
}

void ParticleSystem::renderParticles(FrameInfo& frameInfo) {
	frameIndex = frameInfo.frameIndex;
	pipeline->bind(frameInfo.commandBuffer);

	VkBuffer buffers[] = {particlesBuffers[frameIndex]->getBuffer()};

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, buffers, offsets);

	vkCmdDraw(frameInfo.commandBuffer, PARTICLE_COUNT, 1, 0, 0);
}

void ParticleSystem::updateUniformBuffers(FrameInfo& frameInfo) {
	UniformBufferObject ubo{};

	ubo.deltaTime = frameInfo.frameTime;

	uniformBuffers[frameInfo.frameIndex]->writeToBuffer(&ubo);
}
void ParticleSystem::createParticles() {
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
	for (auto& particle : particles) {
		float r = 0.25f * sqrt(rndDist(rndEngine));
		float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
		float x = r * cos(theta) * 16.0f / 9.0f;
		float y = r * sin(theta);
		particle.position = glm::vec2(x, y);
		particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.25f;
		particle.color =
			glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
	}
}
}  // namespace lvr
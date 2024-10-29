#include "particle_system.h"

#include <cstring>
#include <iostream>
#include <random>

namespace lvr {
ParticleSystem::ParticleSystem(Device& device, VkRenderPass renderPass) : device(device) {
	computeShader = std::make_unique<ComputeShader>(
		device,
		renderPass,
		std::vector<std::string>{"shaders/particles.comp"});
	createPipelineLayout();
	createPipeline(renderPass);
	createParticles();
	createUniformBuffers();
	computeShader->createShaderStorageBuffers<Particle>(particles);
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
	pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	pipelineConfig.attributeDescriptions = Particle::getAttributeDescriptions();
	pipelineConfig.bindingDescriptions = Particle::getBindingDescriptions();
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

void ParticleSystem::renderParticles(FrameInfo& frameInfo) {
	computeShader->beginCompute();
	computeShader->dispatchComputeShader(uniformBuffers, frameInfo);
	computeShader->endCompute();
	frameIndex = frameInfo.frameIndex;
	pipeline->bind(frameInfo.commandBuffer);

	VkBuffer buffers[] = {computeShader->shaderStorageBuffers[frameIndex]->getBuffer()};

	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, buffers, offsets);

	vkCmdDraw(frameInfo.commandBuffer, PARTICLE_COUNT, 1, 0, 0);

	// Purely for debuggging compute shader
	// Buffer stagingBuffer{
	// 	device,
	// 	sizeof(particles[0]),
	// 	particles.size(),
	// 	VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	// 	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	// };
	// device.copyBuffer(
	// 	computeShader->shaderStorageBuffers[frameIndex]->getBuffer(),
	// 	stagingBuffer.getBuffer(),
	// 	(VkDeviceSize)(particles.size() * sizeof(particles[0])));

	// stagingBuffer.map();
	// Particle particle;
	// Particle* ssboBlockPointer = (Particle*)stagingBuffer.getMappedMemory();
	// memcpy(&particle, ssboBlockPointer + 5, sizeof(Particle));
	// // std::cout << particle.position.x << std::endl;
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
		particle.position = glm::vec2(0.5f, 0.5f);
		particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.25f;
		particle.color =
			glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
	}
}
}  // namespace lvr
#pragma once

#include <array>
#include <memory>

#include "../compute_shader.h"

namespace lvr {
const uint32_t PARTICLE_COUNT = 8192;
struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;

	static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Particle);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back(
			{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Particle, position)});
		attributeDescriptions.push_back(
			{1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color)});

		return attributeDescriptions;
	}
};

struct UniformBufferObject {
	float deltaTime = 1.0f;
};

class ParticleSystem {
   public:
	ParticleSystem(Device &device, VkRenderPass renderPass);
	~ParticleSystem();

	void dispatchCompute(FrameInfo &frameInfo, VkCommandBuffer computeCommandBuffer);
	void renderParticles(FrameInfo &frameInfo);

	ParticleSystem(const ParticleSystem &) = delete;
	ParticleSystem &operator=(const ParticleSystem &) = delete;

	int32_t const getFrameIndex() const { return frameIndex; }

	void updateUniformBuffers(FrameInfo &frameInfo);

   private:
	Device &device;

	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
	void createUniformBuffers();
	void createParticles();

	std::unique_ptr<ComputeShader> computeShader;
	std::vector<std::unique_ptr<Buffer>> uniformBuffers;
	std::vector<std::unique_ptr<Buffer>> particlesBuffers;
	std::vector<Particle> particles = std::vector<Particle>(PARTICLE_COUNT);

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout{};
	int32_t frameIndex{0};
};
}  // namespace lvr
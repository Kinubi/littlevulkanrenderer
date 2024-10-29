#pragma once

#include <array>
#include <memory>

#include "../compute_shader.h"

namespace lvr {
const uint32_t PARTICLE_COUNT = 2500;
struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Particle);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Particle, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Particle, color);

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

	void renderParticles(FrameInfo &frameinfo);

	ParticleSystem(const ParticleSystem &) = delete;
	ParticleSystem &operator=(const ParticleSystem &) = delete;

   private:
	Device &device;

	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
	void createUniformDescriptorSet();
	void createUniformBuffers();
	void updateUniformBuffers();

	VkDescriptorSet &uniformDiscriptorSet;
	std::unique_ptr<ComputeShader> computeShader;
	std::vector<std::unique_ptr<Buffer>> uniformBuffers;

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout{};
};
}  // namespace lvr
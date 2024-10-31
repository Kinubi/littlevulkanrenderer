#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "../compute_shader.h"
#include "buffer.h"
#include "device.h"
#include "frameinfo.h"
#include "pipeline.h"

namespace lvr {
const int32_t SPHERE_COUNT = 3;
struct Sphere {
	glm::vec3 center;
	float radius;
	glm::vec3 color;
};

class RayTracingSystem {
	struct UniformBufferObject {
		glm::mat4 viewMatrix{1.0f};
		glm::mat4 inverseViewMatrix{1.0f};
	};

   public:
	RayTracingSystem(Device &device, VkRenderPass renderPass, VkExtent3D extent);
	~RayTracingSystem();

	void dispatchCompute(FrameInfo &frameInfo, VkCommandBuffer computeCommandBuffer);
	void renderRays(FrameInfo &frameInfo);

	RayTracingSystem(const RayTracingSystem &) = delete;
	RayTracingSystem &operator=(const RayTracingSystem &) = delete;

	int32_t const getFrameIndex() const { return frameIndex; }

	void updateUniformBuffers(FrameInfo &frameInfo);
	VkExtent3D getExtent() { return extent; }

   private:
	Device &device;

	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
	void createUniformBuffers();
	void createSpheres();
	void createImage();

	std::unique_ptr<ComputeShader> computeShader;
	std::vector<std::unique_ptr<Buffer>> uniformBuffers;
	std::vector<Sphere> spheres = std::vector<Sphere>(SPHERE_COUNT);
	std::shared_ptr<Texture> image = nullptr;
	std::unique_ptr<DescriptorSetLayout> raytracingSystemLayout{};

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout{};
	int32_t frameIndex{0};
	VkExtent3D extent;
};

}  // namespace lvr
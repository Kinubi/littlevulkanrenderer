#pragma once
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <map>
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
	alignas(16) glm::vec3 center{};
	alignas(16) glm::vec3 color{};
	float radius;
};

class RayTracingSystem {
	struct UniformBufferObject {
		glm::mat4 viewMatrix{1.0f};
		glm::mat4 inverseViewMatrix{1.0f};
		glm::mat4 inverseProjectionMatrix{1.0f};
		int32_t sphereCount{SPHERE_COUNT};
		int32_t frameIndex{0};
	};

   public:
	RayTracingSystem(Device &device, VkRenderPass renderPass, VkExtent3D extent);
	~RayTracingSystem();

	void dispatchCompute(FrameInfo &frameInfo, VkCommandBuffer computeCommandBuffer);
	void renderRays(FrameInfo &frameInfo);

	RayTracingSystem(const RayTracingSystem &) = delete;
	RayTracingSystem &operator=(const RayTracingSystem &) = delete;

	void updateUniformBuffers(FrameInfo &frameInfo, int32_t frameIndex);
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
	std::vector<std::unique_ptr<Buffer>> spheresBuffers;
	std::vector<Sphere> spheres;

	std::vector<std::shared_ptr<Texture>> images;
	std::unique_ptr<DescriptorSetLayout> raytracingSystemLayout{};

	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout{};

	VkExtent3D extent;
};

}  // namespace lvr
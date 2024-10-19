#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "camera.h"
#include "device.h"
#include "gameobject.h"
#include "pipeline.h"

// std

#include <memory>
#include <vector>

namespace lvr {

class SimpleRenderSystem {
   public:
	SimpleRenderSystem(Device &device, VkRenderPass renderPass);
	~SimpleRenderSystem();

	SimpleRenderSystem(const SimpleRenderSystem &) = delete;
	SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

	void renderGameObjects(
		VkCommandBuffer commandBuffer, std::vector<GameObject> &gameObjects, const Camera &camera);

   private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);

	Device &lvrDevice;

	std::unique_ptr<Pipeline> lvrPipeline;
	VkPipelineLayout pipelineLayout{};
};

}  // namespace lvr

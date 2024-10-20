#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "../camera.h"
#include "../device.h"
#include "../frameinfo.h"
#include "../gameobject.h"
#include "../pipeline.h"

// std

#include <memory>
#include <vector>

namespace lvr {

class SimpleRenderSystem {
   public:
	SimpleRenderSystem(
		Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~SimpleRenderSystem();

	SimpleRenderSystem(const SimpleRenderSystem &) = delete;
	SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

	void renderGameObjects(FrameInfo &frameinfo);

   private:
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	Device &lvrDevice;

	std::unique_ptr<Pipeline> lvrPipeline;
	VkPipelineLayout pipelineLayout{};
};

}  // namespace lvr

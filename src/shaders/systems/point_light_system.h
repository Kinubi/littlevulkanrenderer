#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "camera.h"
#include "device.h"
#include "frameinfo.h"
#include "gameobject.h"
#include "pipeline.h"

// std

#include <memory>
#include <vector>

namespace lvr {

class PointLightSystem {
   public:
	PointLightSystem(
		Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~PointLightSystem();

	PointLightSystem(const PointLightSystem &) = delete;
	PointLightSystem &operator=(const PointLightSystem &) = delete;

	void update(FrameInfo &frameInfo, GlobalUbo &ubo);

	void render(FrameInfo &frameinfo);

   private:
	void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void createPipeline(VkRenderPass renderPass);

	Device &lvrDevice;

	std::unique_ptr<Pipeline> lvrPipeline;
	VkPipelineLayout pipelineLayout{};
};

}  // namespace lvr

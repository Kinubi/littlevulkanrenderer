#include "point_light_system.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/gtc/constants.hpp>
#include <vector>

// std

#include <stdexcept>

namespace lvr {

PointLightSystem::PointLightSystem(
	Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	: lvrDevice(device) {
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem() {
	vkDestroyPipelineLayout(lvrDevice.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
	// VkPushConstantRange pushConstantRange{};
	// pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	// pushConstantRange.offset = 0;
	// pushConstantRange.size = sizeof(SimplePushConstantData);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(lvrDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void PointLightSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	lvrPipeline = std::make_unique<Pipeline>(
		lvrDevice,
		"shaders/point_light.vert.spv",
		"shaders/point_light.frag.spv",
		pipelineConfig);
}

void PointLightSystem::renderGameObjects(FrameInfo &frameInfo) {
	lvrPipeline->bind(frameInfo.commandBuffer);

	auto projectionView = frameInfo.camera.getProjection() * frameInfo.camera.getView();

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr);

	vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
}

}  // namespace lvr
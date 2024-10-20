#include "simplerendersystem.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/gtc/constants.hpp>
#include <vector>

#include "camera.h"
#include "device.h"
#include "gameobject.h"
#include "pipeline.h"

// std

#include <memory>
#include <stdexcept>

namespace lvr {

struct SimplePushConstantData {
	glm::mat4 transform{1.0f};
	alignas(16) glm::vec4 color;
};

SimpleRenderSystem::SimpleRenderSystem(Device &device, VkRenderPass renderPass)
	: lvrDevice(device) {
	createPipelineLayout();
	createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
	vkDestroyPipelineLayout(lvrDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout() {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(lvrDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	lvrPipeline = std::make_unique<Pipeline>(
		lvrDevice,
		"shaders/simple_shader.vert.spv",
		"shaders/simple_shader.frag.spv",
		pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(
	VkCommandBuffer commandBuffer, std::vector<GameObject> &gameObjects, const Camera &camera) {
	lvrPipeline->bind(commandBuffer);

	auto projectionView = camera.getProjection() * camera.getView();

	for (auto &obj : gameObjects) {
		obj.tranform.rotation.y = glm::mod(obj.tranform.rotation.y + 0.001f, glm::two_pi<float>());
		// obj.tranform.rotation.x = glm::mod(obj.tranform.rotation.x +0.001f,
		// glm::two_pi<float>());
		SimplePushConstantData push{};
		push.color = obj.color;
		push.transform = projectionView * obj.tranform.mat4();

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);
		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer);
	}
}

}  // namespace lvr
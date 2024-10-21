#include "point_light_system.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <vector>

// std

#include <stdexcept>

namespace lvr {

struct PointLightPushConstants {
	glm::vec4 position{};
	glm::vec4 color{};
	float radius;
};

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
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PointLightPushConstants);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

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

void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
	auto rotateLight = glm::rotate(glm::mat4(1.0f), frameInfo.frameTime, {0.0f, -1.0f, 0.0f});
	int32_t lightIndex = 0;
	for (auto &kv : frameInfo.gameObjects) {
		auto &obj = kv.second;
		if (obj.pointLight == nullptr) continue;

		assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

		obj.tranform.translation =
			glm::vec3(rotateLight * glm::vec4(obj.tranform.translation, 1.0f));
		ubo.pointLights[lightIndex].position = glm::vec4(obj.tranform.translation, 1.0f);
		ubo.pointLights[lightIndex].color =
			glm::vec4(obj.color.x, obj.color.y, obj.color.z, obj.pointLight->lightIntensity);

		lightIndex++;
	}

	ubo.numLights = lightIndex;
}

void PointLightSystem::render(FrameInfo &frameInfo) {
	lvrPipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr);

	for (auto &kv : frameInfo.gameObjects) {
		auto &obj = kv.second;
		if (obj.pointLight == nullptr) continue;

		PointLightPushConstants push{};
		push.position = glm::vec4(obj.tranform.translation, 1.0f);
		push.color =
			glm::vec4(obj.color.x, obj.color.y, obj.color.z, obj.pointLight->lightIntensity);
		push.radius = obj.tranform.scale.x;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(PointLightPushConstants),
			&push);

		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}
}

}  // namespace lvr
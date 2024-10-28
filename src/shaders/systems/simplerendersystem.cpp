#include "simplerendersystem.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/gtc/constants.hpp>
#include <iostream>
#include <vector>

// std

#include <stdexcept>

namespace lvr {

struct SimplePushConstantData {
	glm::mat4 modelMatrix{1.0f};
	glm::mat4 normalMatrix{1.0f};
};

SimpleRenderSystem::SimpleRenderSystem(
	Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	: lvrDevice(device) {
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
	vkDestroyPipelineLayout(lvrDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	renderSystemLayout =
		DescriptorSetLayout::Builder(lvrDevice)
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		globalSetLayout,
		renderSystemLayout->getDescriptorSetLayout()};

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

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig, lvrDevice.getMsaaSamples());

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	lvrPipeline = std::make_unique<Pipeline>(
		lvrDevice,
		std::vector<std::string>{
			"shaders/simple_shader.vert",
			"shaders/simple_shader.frag",
		},
		pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
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

	for (auto& kv : frameInfo.gameObjects) {
		auto& obj = kv.second;
		if (obj.model == nullptr) continue;

		auto bufferInfo = obj.getBufferInfo(frameInfo.frameIndex);
		auto imageInfo = obj.diffuseMap->getImageInfo();
		VkDescriptorSet gameObjectDescriptorSet;
		DescriptorWriter(*renderSystemLayout, frameInfo.frameDescriptorPool)
			.writeBuffer(0, &bufferInfo)
			.writeImage(1, &imageInfo)
			.build(gameObjectDescriptorSet);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			1,	// starting set (0 is the globalDescriptorSet, 1 is the set specific to this	//
				// system)
			1,	// set count
			&gameObjectDescriptorSet,
			0,
			nullptr);

		SimplePushConstantData push{};
		push.modelMatrix = obj.transform.mat4();
		push.normalMatrix = obj.transform.normalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);

		obj.model->bind(frameInfo.commandBuffer);
		obj.model->draw(frameInfo.commandBuffer);
	}
}

}  // namespace lvr

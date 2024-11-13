#include "ray_tracing_system.h"

#include <random>

namespace lvr {
RayTracingSystem::RayTracingSystem(Device& device, VkRenderPass renderPass, VkExtent3D extent)
	: device(device), extent(extent) {
	createPipelineLayout();
	createPipeline(renderPass);
	computeShader = std::make_unique<ComputeShader>(
		device,
		renderPass,
		std::vector<std::string>{"shaders/raytracing.comp"},
		DescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.build());
	materialManager = std::make_unique<MaterialManager>(device);
	createSpheres();
	createUniformBuffers();
	spheresBuffers = computeShader->createShaderStorageBuffers<Sphere>(spheres);
	createImage();
}

RayTracingSystem::~RayTracingSystem() {
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RayTracingSystem::dispatchCompute(FrameInfo& frameInfo, VkCommandBuffer computeCommandBuffer) {
	for (auto image : images) {
		image->transitionLayout(
			computeCommandBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_GENERAL,
			true);
	}

	VkDescriptorSet computeDescriptorSet;
	auto bufferInfoubo = uniformBuffers[frameInfo.frameIndex]->descriptorInfo();
	auto imageInfoLastFrame =
		images[abs((frameInfo.frameIndex - 1) % SwapChain::MAX_FRAMES_IN_FLIGHT)]->getImageInfo();
	auto imageInfo = images[frameInfo.frameIndex]->getImageInfo();
	auto bufferInfoCurrentFrame = spheresBuffers[frameInfo.frameIndex]->descriptorInfo();

	DescriptorWriter(*computeShader->getComputeShaderLayout(), frameInfo.frameDescriptorPool)
		.writeBuffer(0, &bufferInfoubo)
		.writeBuffer(1, &bufferInfoCurrentFrame)
		.writeImage(2, &imageInfoLastFrame)
		.writeImage(3, &imageInfo)
		.build(computeDescriptorSet);

	computeShader->dispatchComputeShader(
		computeCommandBuffer,
		computeDescriptorSet,
		glm::vec2((extent.width / 16) + 1, (extent.height / 16) + 1));
	for (auto image : images) {
		image->transitionLayout(
			computeCommandBuffer,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_GENERAL,
			false);
	}
}

void RayTracingSystem::renderRays(FrameInfo& frameInfo) {
	pipeline->bind(frameInfo.commandBuffer);

	VkDescriptorSet raytracingDescriptorSet;

	auto imageInfo = images[frameInfo.frameIndex]->getImageInfo();

	DescriptorWriter(*raytracingSystemLayout, frameInfo.frameDescriptorPool)
		.writeImage(0, &imageInfo)
		.build(raytracingDescriptorSet);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,	// starting set (0 is the globalDescriptorSet, 1 is the set specific to this	//
			// system)
		1,	// set count
		&raytracingDescriptorSet,
		0,
		nullptr);

	vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
}

void RayTracingSystem::updateUniformBuffers(FrameInfo& frameInfo, int32_t frameIndex) {
	UniformBufferObject ubo{};
	ubo.viewMatrix = frameInfo.camera.getView();
	ubo.inverseViewMatrix = frameInfo.camera.getInverseView();
	ubo.inverseProjectionMatrix = glm::inverse(frameInfo.camera.getProjection());
	ubo.frameIndex = frameIndex;
	uniformBuffers[frameInfo.frameIndex]->writeToBuffer(&ubo);
}

void RayTracingSystem::createPipelineLayout() {
	raytracingSystemLayout =
		DescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
		raytracingSystemLayout->getDescriptorSetLayout()};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void RayTracingSystem::createPipeline(VkRenderPass renderPass) {
	assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig, device.getMsaaSamples());
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	Pipeline::enableAlphaBlending(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<Pipeline>(
		device,
		std::vector<std::string>{"shaders/raytracing.frag", "shaders/raytracing.vert"},
		pipelineConfig);
}

void RayTracingSystem::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		uniformBuffers[i] = std::make_unique<Buffer>(
			device,
			bufferSize,
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		;
		uniformBuffers[i]->map();
	}
}

void RayTracingSystem::createSpheres() {
	spheres.reserve(SPHERE_COUNT);
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	{
		Sphere sphere;
		sphere.center = {0.0f, 11.0f, -5.0f};
		sphere.radius = 10.0f;
		sphere.color = glm::vec3(0.82f, 0.5f, 0.2f);

		MaterialAsset materialAsset;
		materialAsset.setAlbedo(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveColor(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveStrength(0.5f);
		materialAsset.setRoughness(0.5f);
		materialAsset.setMetalness(0.5f);
		materialAsset.setRefractiveIndex(1.0f);

		sphere.materialID = materialManager->create(materialAsset);
		spheres.emplace_back(sphere);
	}

	{
		Sphere sphere;
		sphere.center = {2.0f, 0.0f, 0.0f};
		sphere.radius = 1.0f;
		sphere.color = glm::vec3(0.2f, 0.3f, 1.0f);

		MaterialAsset materialAsset;
		materialAsset.setAlbedo(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveColor(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveStrength(0.5f);
		materialAsset.setRoughness(0.5f);
		materialAsset.setMetalness(0.5f);
		materialAsset.setRefractiveIndex(1.0f);

		sphere.materialID = materialManager->create(materialAsset);
		spheres.emplace_back(sphere);
	}

	{
		Sphere sphere;
		sphere.center = {0.0f, -101.0f, 0.0f};
		sphere.radius = 100.0f;
		sphere.color = glm::vec3(1.0f, 0.0f, 1.0f);

		MaterialAsset materialAsset;
		materialAsset.setAlbedo(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveColor(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveStrength(0.5f);
		materialAsset.setRoughness(0.5f);
		materialAsset.setMetalness(0.5f);
		materialAsset.setRefractiveIndex(1.0f);

		sphere.materialID = materialManager->create(materialAsset);
		spheres.emplace_back(sphere);
	}

	{
		Sphere sphere;
		sphere.center = {0.0f, 0.0f, 0.0f};
		sphere.radius = 1.0f;
		sphere.color = glm::vec3(1.0f, 1.0f, 1.0f);

		MaterialAsset materialAsset;
		materialAsset.setAlbedo(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveColor(glm::vec3(0.82f, 0.5f, 0.2f));
		materialAsset.setEmissiveStrength(0.5f);
		materialAsset.setRoughness(0.5f);
		materialAsset.setMetalness(0.5f);
		materialAsset.setRefractiveIndex(1.0f);

		sphere.materialID = materialManager->create(materialAsset);
		spheres.emplace_back(sphere);
	}
}

void RayTracingSystem::createImage() {
	images.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		images[i] = std::move(std::make_shared<Texture>(
			device,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			extent,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_SAMPLE_COUNT_1_BIT));
	}
}

}  // namespace lvr

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
			.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.build());

	createSpheres();
	createUniformBuffers();
	computeShader->createShaderStorageBuffers<Sphere>(spheres);
	createImage();
}

RayTracingSystem::~RayTracingSystem() {
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void RayTracingSystem::dispatchCompute(FrameInfo& frameInfo, VkCommandBuffer computeCommandBuffer) {
	image->transitionLayout(
		computeCommandBuffer,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		true);
	computeShader->dispatchComputeShader(
		uniformBuffers,
		frameInfo,
		computeCommandBuffer,
		image->getImageInfo(),
		glm::vec2((extent.width / 16) + 1, (extent.height / 16) + 1));
	image->transitionLayout(
		computeCommandBuffer,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_GENERAL,
		false);
}

void RayTracingSystem::renderRays(FrameInfo& frameInfo) {
	frameIndex = frameInfo.frameIndex;
	pipeline->bind(frameInfo.commandBuffer);

	VkDescriptorSet raytracingDescriptorSet;

	auto imageInfo = image->getImageInfo();

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

void RayTracingSystem::updateUniformBuffers(FrameInfo& frameInfo) {
	UniformBufferObject ubo{};
	ubo.viewMatrix = frameInfo.camera.getView();
	ubo.inverseViewMatrix = frameInfo.camera.getInverseView();
	ubo.inverseProjectionMatrix = glm::inverse(frameInfo.camera.getProjection());

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
	spheres.resize(SPHERE_COUNT);
	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
	for (auto& sphere : spheres) {
		sphere.center =
			2.0f * glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
		sphere.radius = 0.5f * rndDist(rndEngine);	// Example radius
		sphere.color = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
	}
}

void RayTracingSystem::createImage() {
	image = std::make_shared<Texture>(
		device,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		extent,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SAMPLE_COUNT_1_BIT);
}

}  // namespace lvr

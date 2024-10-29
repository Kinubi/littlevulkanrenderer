#include "application.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include "descriptors.h"
#include "device.h"
#include "frameinfo.h"
#include "gameobject.h"
#include "iostream"
#include "keyboard_movement_controller.h"
#include "model.h"
#include "swapchain.h"
#include "window.h"

// std

#include <chrono>
#include <memory>

namespace lvr {

Application::Application() {
	globalPool =
		DescriptorPool::Builder(lvrDevice)
			.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

	// build frame descriptor pools
	framePools.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	auto framePoolBuilder = DescriptorPool::Builder(lvrDevice)
								.setMaxSets(1000)
								.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
								.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
								.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	for (int i = 0; i < framePools.size(); i++) {
		framePools[i] = framePoolBuilder.build();
	}

	loadGameObjects();
}

Application::~Application() {}

void Application::OnStart() {
	for (int32_t i = 0; i < uboBuffers.size(); i++) {
		uboBuffers[i] = std::make_unique<Buffer>(
			lvrDevice,
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			lvrDevice.properties.limits.minUniformBufferOffsetAlignment);

		uboBuffers[i]->map();
	}

	auto globalSetLayout =
		DescriptorSetLayout::Builder(lvrDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

	for (int32_t i = 0; i < globalDescriptorSets.size(); i++) {
		auto bufferInfo = uboBuffers[i]->descriptorInfo();
		DescriptorWriter(*globalSetLayout, *globalPool)
			.writeBuffer(0, &bufferInfo)
			.build(globalDescriptorSets[i]);
	}
	simpleRenderSystem = std::make_unique<SimpleRenderSystem>(
		lvrDevice,
		lvrRenderer.getSwapChainRenderPass(),
		globalSetLayout->getDescriptorSetLayout());

	pointLightSystem = std::make_unique<PointLightSystem>(
		lvrDevice,
		lvrRenderer.getSwapChainRenderPass(),
		globalSetLayout->getDescriptorSetLayout());

	viewerObject.transform.translation.z = -2.5f;

	auto currentTime = std::chrono::high_resolution_clock::now();
	while (!lvrWIndow.shouldClose()) {
		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime =
			std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime)
				.count();
		currentTime = newTime;

		OnUpdate(frameTime);
	}

	vkDeviceWaitIdle(lvrDevice.device());
}

void Application::OnUpdate(float dt) {
	glfwPollEvents();

	cameraController.moveInPlaneXZ(lvrWIndow.getGLFWWindow(), dt, viewerObject);
	camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

	float aspect = lvrRenderer.getAspectRatio();

	camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

	if (auto commandBuffer = lvrRenderer.beginFrame()) {
		int32_t frameIndex = lvrRenderer.getFrameIndex();
		framePools[frameIndex]->resetPool();

		FrameInfo frameInfo{
			frameIndex,
			dt,
			commandBuffer,
			camera,
			globalDescriptorSets[frameIndex],
			*framePools[frameIndex],
			gameObjectManager.gameObjects};

		// update

		GlobalUbo ubo{};
		ubo.projectionMatrix = camera.getProjection();
		ubo.viewMatrix = camera.getView();
		ubo.inverseViewMatrix = camera.getInverseView();
		pointLightSystem->update(frameInfo, ubo);

		uboBuffers[frameIndex]->writeToBuffer(&ubo);
		uboBuffers[frameIndex]->flush();
		gameObjectManager.updateBuffer(frameIndex);

		lvrRenderer.beginSwapChainRenderPass(commandBuffer);

		simpleRenderSystem->renderGameObjects(frameInfo);
		pointLightSystem->render(frameInfo);
		lvrRenderer.endSwapChainRenderPass(commandBuffer);
		lvrRenderer.endFrame();
	}
}

void Application::loadGameObjects() {
	std::shared_ptr<Model> smoothModel =
		Model::createModelFromFile(lvrDevice, "models/smooth_vase.obj");

	auto& smoothObject = gameObjectManager.createGameObject();
	smoothObject.model = smoothModel;
	smoothObject.transform.translation = {-0.5f, 0.5f, 0.0f};
	smoothObject.transform.scale = {0.5f, 0.5f, 0.5f};

	std::shared_ptr<Model> flatModel =
		Model::createModelFromFile(lvrDevice, "models/flat_vase.obj");

	std::shared_ptr<Texture> marbleTexture =
		Texture::createTextureFromFile(lvrDevice, "textures/missing.png");
	auto& flatObject = gameObjectManager.createGameObject();
	flatObject.model = flatModel;
	flatObject.diffuseMap = marbleTexture;
	flatObject.transform.translation = {0.5f, 0.5f, 0.0f};
	flatObject.transform.scale = {0.5f, 0.5f, 0.5f};

	std::shared_ptr<Model> cubeModel =
		Model::createModelFromFile(lvrDevice, "models/colored_cube.obj");

	auto& cubeObject = gameObjectManager.createGameObject();
	cubeObject.model = cubeModel;
	cubeObject.transform.translation = {0.0f, 1.0f, 0.0f};
	cubeObject.transform.scale = {0.5f, 0.5f, 0.5f};

	std::shared_ptr<Model> quadModel = Model::createModelFromFile(lvrDevice, "models/quad.obj");

	auto& quadObject = gameObjectManager.createGameObject();
	quadObject.model = quadModel;
	quadObject.transform.translation = {0.0f, 0.5f, 0.0f};
	quadObject.transform.scale = {1.5f, 1.5f, 1.5f};

	std::shared_ptr<Model> humanModel =
		Model::createModelFromFile(lvrDevice, "models/FinalBaseMesh.obj");

	auto& humanObject = gameObjectManager.createGameObject();
	humanObject.model = humanModel;
	humanObject.transform.translation = {0.0f, 0.5f, 0.0f};
	humanObject.transform.rotation = {0.0f, 0.0f, 0.5 * glm::two_pi<float>()};
	humanObject.transform.scale = {0.1f, 0.1f, 0.1};

	std::vector<glm::vec3> lightColors{
		{1.f, .1f, .1f},
		{.1f, .1f, 1.f},
		{.1f, 1.f, .1f},
		{1.f, 1.f, .1f},
		{.1f, 1.f, 1.f},
		{1.f, 1.f, 1.f}	 //
	};

	for (int32_t i = 0; i < lightColors.size(); i++) {
		auto& pointLight = gameObjectManager.makePointLight(0.2f);
		pointLight.color = glm::vec4(lightColors[i], 1.0f);
		auto rotateLight = glm::rotate(
			glm::mat4(1.0f),
			(i * glm::two_pi<float>()) / lightColors.size(),
			{0.0f, -1.0f, 0.0f});

		pointLight.transform.translation =
			glm::vec3(rotateLight * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f));
	}
}

}  // namespace lvr
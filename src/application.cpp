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
#include "systems/point_light_system.h"
#include "systems/simplerendersystem.h"
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

	viewerObject.tranform.translation.z = -2.5f;

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
	camera.setViewYXZ(viewerObject.tranform.translation, viewerObject.tranform.rotation);

	float aspect = lvrRenderer.getAspectRatio();

	camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);

	if (auto commandBuffer = lvrRenderer.beginFrame()) {
		int32_t frameIndex = lvrRenderer.getFrameIndex();

		FrameInfo frameInfo{
			frameIndex,
			dt,
			commandBuffer,
			camera,
			globalDescriptorSets[frameIndex],
			gameObjects};

		// update

		GlobalUbo ubo{};
		ubo.projectionMatrix = camera.getProjection();
		ubo.viewMatrix = camera.getView();
		ubo.inverseViewMatrix = camera.getInverseView();
		pointLightSystem->update(frameInfo, ubo);

		uboBuffers[frameIndex]->writeToBuffer(&ubo);
		uboBuffers[frameIndex]->flush();

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

	auto smoothObject = GameObject::createGameObject();
	smoothObject.model = smoothModel;
	smoothObject.tranform.translation = {-0.5f, 0.5f, 0.0f};
	smoothObject.tranform.scale = {0.5f, 0.5f, 0.5f};

	gameObjects.emplace(smoothObject.getId(), std::move(smoothObject));

	std::shared_ptr<Model> flatModel =
		Model::createModelFromFile(lvrDevice, "models/flat_vase.obj");

	auto flatObject = GameObject::createGameObject();
	flatObject.model = flatModel;
	flatObject.tranform.translation = {0.5f, 0.5f, 0.0f};
	flatObject.tranform.scale = {0.5f, 0.5f, 0.5f};

	gameObjects.emplace(flatObject.getId(), std::move(flatObject));

	std::shared_ptr<Model> quadModel = Model::createModelFromFile(lvrDevice, "models/quad.obj");

	auto quadObject = GameObject::createGameObject();
	quadObject.model = quadModel;
	quadObject.tranform.translation = {0.0f, 0.5f, 0.0f};
	quadObject.tranform.scale = {1.5f, 1.5f, 1.5f};

	gameObjects.emplace(quadObject.getId(), std::move(quadObject));

	std::shared_ptr<Model> humanModel =
		Model::createModelFromFile(lvrDevice, "models/FinalBaseMesh.obj");

	auto humanObject = GameObject::createGameObject();
	humanObject.model = humanModel;
	humanObject.tranform.translation = {0.0f, 0.5f, 0.0f};
	humanObject.tranform.rotation = {0.0f, 0.0f, 0.5 * glm::two_pi<float>()};
	humanObject.tranform.scale = {0.1f, 0.1f, 0.1};

	gameObjects.emplace(humanObject.getId(), std::move(humanObject));

	std::vector<glm::vec3> lightColors{
		{1.f, .1f, .1f},
		{.1f, .1f, 1.f},
		{.1f, 1.f, .1f},
		{1.f, 1.f, .1f},
		{.1f, 1.f, 1.f},
		{1.f, 1.f, 1.f}	 //
	};

	for (int32_t i = 0; i < lightColors.size(); i++) {
		auto pointLight = GameObject::makePointLight(0.2f);
		pointLight.color = glm::vec4(lightColors[i], 1.0f);
		auto rotateLight = glm::rotate(
			glm::mat4(1.0f),
			(i * glm::two_pi<float>()) / lightColors.size(),
			{0.0f, -1.0f, 0.0f});

		pointLight.tranform.translation =
			glm::vec3(rotateLight * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f));
		gameObjects.emplace(pointLight.getId(), std::move(pointLight));
	}
}

}  // namespace lvr
#include "application.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include "device.h"
#include "gameobject.h"
#include "keyboard_movement_controller.h"
#include "model.h"
#include "simplerendersystem.h"
#include "window.h"

// std

#include <chrono>
#include <memory>

namespace lvr {

Application::Application() { loadGameObjects(); }

Application::~Application() {}

void Application::OnStart() {
	camera.setViewDirection(glm::vec3{0.0f}, glm::vec3{0.5f, 0.0f, 1.0f});

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
		lvrRenderer.beginSwapChainRenderPass(commandBuffer);
		simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
		lvrRenderer.endSwapChainRenderPass(commandBuffer);
		lvrRenderer.endFrame();
	}
}

void Application::loadGameObjects() {
	std::shared_ptr<Model> lvrModel =
		Model::createModelFromFile(lvrDevice, "models/smooth_vase.obj");

	auto gameObject = GameObject::createGameObject();
	gameObject.model = lvrModel;
	gameObject.tranform.translation = {0.0f, 0.0f, 2.5f};
	gameObject.tranform.scale = {0.5f, 0.5f, 0.5f};

	gameObjects.push_back(std::move(gameObject));
}

}  // namespace lvr
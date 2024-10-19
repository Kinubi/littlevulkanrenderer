#include "application.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <vector>

#include "device.h"
#include "gameobject.h"
#include "model.h"
#include "pipeline.h"
#include "simplerendersystem.h"
#include "swapchain.h"
#include "window.h"

// std
#include <array>
#include <memory>
#include <stdexcept>

namespace lvr {

Application::Application() { loadGameObjects(); }

Application::~Application() {}

void Application::OnStart() {
	camera.setViewDirection(glm::vec3{0.0f}, glm::vec3{0.5f, 0.0f, 1.0f});
	while (!lvrWIndow.shouldClose()) {
		OnUpdate();
	}

	vkDeviceWaitIdle(lvrDevice.device());
}

void Application::OnUpdate() {
	glfwPollEvents();
	float aspect = lvrRenderer.getAspectRatio();
	// camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
	camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 10.0f);
	if (auto commandBuffer = lvrRenderer.beginFrame()) {
		lvrRenderer.beginSwapChainRenderPass(commandBuffer);
		simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
		lvrRenderer.endSwapChainRenderPass(commandBuffer);
		lvrRenderer.endFrame();
	}
}

// temporary helper function, creates a 1x1x1 cube centered at offset
std::unique_ptr<Model> createCubeModel(Device &device, glm::vec3 offset) {
	std::vector<Model::Vertex> vertices{

		// left face (white)
		{{-.5f, -.5f, -.5f}, {.9f, .9f, .9f, 1.0f}},
		{{-.5f, .5f, .5f}, {.9f, .9f, .9f, 1.0f}},
		{{-.5f, -.5f, .5f}, {.9f, .9f, .9f, 1.0f}},
		{{-.5f, -.5f, -.5f}, {.9f, .9f, .9f, 1.0f}},
		{{-.5f, .5f, -.5f}, {.9f, .9f, .9f, 1.0f}},
		{{-.5f, .5f, .5f}, {.9f, .9f, .9f, 1.0f}},

		// right face (yellow)
		{{.5f, -.5f, -.5f}, {.8f, .8f, .1f, 1.0f}},
		{{.5f, .5f, .5f}, {.8f, .8f, .1f, 1.0f}},
		{{.5f, -.5f, .5f}, {.8f, .8f, .1f, 1.0f}},
		{{.5f, -.5f, -.5f}, {.8f, .8f, .1f, 1.0f}},
		{{.5f, .5f, -.5f}, {.8f, .8f, .1f, 1.0f}},
		{{.5f, .5f, .5f}, {.8f, .8f, .1f, 1.0f}},

		// top face (orange, remember y axis points down)
		{{-.5f, -.5f, -.5f}, {.9f, .6f, .1f, 1.0f}},
		{{.5f, -.5f, .5f}, {.9f, .6f, .1f, 1.0f}},
		{{-.5f, -.5f, .5f}, {.9f, .6f, .1f, 1.0f}},
		{{-.5f, -.5f, -.5f}, {.9f, .6f, .1f, 1.0f}},
		{{.5f, -.5f, -.5f}, {.9f, .6f, .1f, 1.0f}},
		{{.5f, -.5f, .5f}, {.9f, .6f, .1f, 1.0f}},

		// bottom face (red)
		{{-.5f, .5f, -.5f}, {.8f, .1f, .1f, 1.0f}},
		{{.5f, .5f, .5f}, {.8f, .1f, .1f, 1.0f}},
		{{-.5f, .5f, .5f}, {.8f, .1f, .1f, 1.0f}},
		{{-.5f, .5f, -.5f}, {.8f, .1f, .1f, 1.0f}},
		{{.5f, .5f, -.5f}, {.8f, .1f, .1f, 1.0f}},
		{{.5f, .5f, .5f}, {.8f, .1f, .1f, 1.0f}},

		// nose face (blue)
		{{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f, 1.0f}},
		{{.5f, .5f, 0.5f}, {.1f, .1f, .8f, 1.0f}},
		{{-.5f, .5f, 0.5f}, {.1f, .1f, .8f, 1.0f}},
		{{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f, 1.0f}},
		{{.5f, -.5f, 0.5f}, {.1f, .1f, .8f, 1.0f}},
		{{.5f, .5f, 0.5f}, {.1f, .1f, .8f, 1.0f}},

		// tail face (green)
		{{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f, 1.0f}},
		{{.5f, .5f, -0.5f}, {.1f, .8f, .1f, 1.0f}},
		{{-.5f, .5f, -0.5f}, {.1f, .8f, .1f, 1.0f}},
		{{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f, 1.0f}},
		{{.5f, -.5f, -0.5f}, {.1f, .8f, .1f, 1.0f}},
		{{.5f, .5f, -0.5f}, {.1f, .8f, .1f, 1.0f}},

	};
	for (auto &v : vertices) {
		v.position += offset;
	}
	return std::make_unique<Model>(device, vertices);
}

void Application::loadGameObjects() {
	std::shared_ptr<Model> lvrModel = createCubeModel(lvrDevice, {0.0f, 0.0f, 0.0f});

	auto cube = GameObject::createGameObject();
	cube.model = lvrModel;
	cube.tranform.translation = {0.0f, 0.0f, 2.5f};
	cube.tranform.scale = {0.5f, 0.5f, 0.5f};

	gameObjects.push_back(std::move(cube));
}

}  // namespace lvr
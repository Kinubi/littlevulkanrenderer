#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "camera.h"
#include "descriptors.h"
#include "device.h"
#include "gameobject.h"
#include "keyboard_movement_controller.h"
#include "renderer.h"
#include "swapchain.h"
#include "systems/point_light_system.h"
#include "systems/simplerendersystem.h"
#include "window.h"

// std

#include <memory>
#include <vector>

namespace lvr {

struct GlobalUbo {
	glm::mat4 projectionMatrix{1.0f};
	glm::mat4 viewMatrix{1.0f};
	glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f};
	glm::vec3 lightPosition{-1.0f};
	alignas(16) glm::vec4 lightColor{1.0f};
};

class Application {
   public:
	static constexpr uint32_t WIDTH = 1280;
	static constexpr uint32_t HEIGHT = 720;

	Application();
	~Application();

	Application(const Application &) = delete;
	Application &operator=(const Application &) = delete;

	void OnStart();
	void OnUpdate(float dt);

   private:
	void loadGameObjects();

	Window lvrWIndow{WIDTH, HEIGHT, "LVR"};
	Device lvrDevice{lvrWIndow};
	Renderer lvrRenderer{lvrWIndow, lvrDevice};

	std::unique_ptr<SimpleRenderSystem> simpleRenderSystem;
	std::unique_ptr<PointLightSystem> pointLightSystem;

	std::unique_ptr<DescriptorPool> globalPool{};
	GameObject::Map gameObjects;
	Camera camera{};

	GameObject viewerObject = GameObject::createGameObject();
	KeyboardMovementController cameraController{};

	std::vector<std::unique_ptr<Buffer>> uboBuffers =
		std::vector<std::unique_ptr<Buffer>>(SwapChain::MAX_FRAMES_IN_FLIGHT);

	std::vector<VkDescriptorSet> globalDescriptorSets =
		std::vector<VkDescriptorSet>(SwapChain::MAX_FRAMES_IN_FLIGHT);
};

}  // namespace lvr

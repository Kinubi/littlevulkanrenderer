#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "camera.h"
#include "descriptors.h"
#include "device.h"
#include "gameobject.h"
#include "keyboard_movement_controller.h"
#include "renderer.h"
#include "shaders/compute_shader_manager.h"
#include "shaders/systems/particle_system.h"
#include "shaders/systems/point_light_system.h"
#include "shaders/systems/ray_tracing_system.h"
#include "shaders/systems/simplerendersystem.h"
#include "swapchain.h"
#include "window.h"

// std

#include <memory>
#include <vector>

namespace lvr {

class Application {
   public:
	static constexpr uint32_t WIDTH = 1280;
	static constexpr uint32_t HEIGHT = 720;

	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void OnStart();
	void OnUpdate(float dt);

   private:
	void loadGameObjects();

	Window lvrWIndow{WIDTH, HEIGHT, "LVR"};
	Device lvrDevice{lvrWIndow};
	Renderer lvrRenderer{lvrWIndow, lvrDevice};
	ComputeShaderManager computeShaderManager{lvrDevice};
	std::unique_ptr<SimpleRenderSystem> simpleRenderSystem;
	std::unique_ptr<PointLightSystem> pointLightSystem;
	// std::unique_ptr<ParticleSystem> particleSystem;
	std::unique_ptr<RayTracingSystem> raytracingSystem;

	std::unique_ptr<DescriptorPool> globalPool{};
	std::vector<std::unique_ptr<DescriptorPool>> framePools;
	GameObjectManager gameObjectManager{lvrDevice};
	Camera camera{};

	GameObject& viewerObject = gameObjectManager.createGameObject();

	KeyboardMovementController cameraController{};

	std::vector<std::unique_ptr<Buffer>> uboBuffers =
		std::vector<std::unique_ptr<Buffer>>(SwapChain::MAX_FRAMES_IN_FLIGHT);

	std::vector<VkDescriptorSet> globalDescriptorSets =
		std::vector<VkDescriptorSet>(SwapChain::MAX_FRAMES_IN_FLIGHT);
	int32_t frameRayIndex{0};
};

}  // namespace lvr

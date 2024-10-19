#pragma once

#include "device.h"
#include "gameobject.h"
#include "pipeline.h"
#include "swapchain.h"
#include "window.h"

#include <cstdint>
#include <vulkan/vulkan_core.h>

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

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  void OnStart();
  void OnUpdate();

private:
  void loadGameObjects();
  void createPipelineLayout();
  void createPipeline();
  void createCommandBuffers();
  void freeCommandBuffers();
  void drawFrame();
  void recreateSwapChain();
  void recordCommandBuffer(uint32_t imageIndex);
  void renderGameObjects(VkCommandBuffer commandBuffer);

  Window lvrWIndow{WIDTH, HEIGHT, "LVR"};
  Device lvrDevice{lvrWIndow};
  std::unique_ptr<SwapChain> lvrSwapChain;
  std::unique_ptr<Pipeline> lvrPipeline;
  VkPipelineLayout pipelineLayout{};
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<GameObject> gameObjects;
};

} // namespace lvr

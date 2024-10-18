#pragma once

#include "device.h"
#include "pipeline.h"
#include "swapchain.h"
#include "window.h"

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
  void createPipelineLayout();
  void createPipeline();
  void createCommandBuffers();
  void drawFrame();
  Window lvrWIndow{WIDTH, HEIGHT, "LVR"};
  Device lvrDevice{lvrWIndow};
  SwapChain lvrSwapChain{lvrDevice, lvrWIndow.getExtent()};
  std::unique_ptr<Pipeline> lvrPipeline;
  VkPipelineLayout pipelineLayout{};
  std::vector<VkCommandBuffer> commandBuffers;
};

} // namespace lvr


#pragma once

#include "camera.h"
#include "device.h"
#include "gameobject.h"
#include "renderer.h"
#include "simplerendersystem.h"
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

  Window lvrWIndow{WIDTH, HEIGHT, "LVR"};
  Device lvrDevice{lvrWIndow};
  Renderer lvrRenderer{lvrWIndow, lvrDevice};
  SimpleRenderSystem simpleRenderSystem{lvrDevice,
                                        lvrRenderer.getSwapChainRenderPass()};

  std::vector<GameObject> gameObjects;
  Camera camera{};
};

} // namespace lvr

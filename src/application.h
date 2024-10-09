#pragma once

#include "window.h"

namespace lvr {

class Application {
public:
  static constexpr u_int32_t WIDTH = 1280;
  static constexpr u_int32_t HEIGHT = 720;

  void OnStart();
  void OnUpdate();

private:
  Window lvrWIndow{WIDTH, HEIGHT, "LVR"};
};

} // namespace lvr
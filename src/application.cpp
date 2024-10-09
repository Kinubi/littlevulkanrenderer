#include "application.h"
#include "window.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace lvr {

void Application::OnStart() {

  while (!lvrWIndow.shouldClose()) {

    OnUpdate();
  }
}

void Application::OnUpdate() {
  glfwPollEvents();
  // std::cout << "Updating" << std::endl;
}
} // namespace lvr
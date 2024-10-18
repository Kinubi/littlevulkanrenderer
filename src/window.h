#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

#include "vulkan/vulkan_wayland.h"
#include <wayland-client.h>

// #include "platform/waylandwindow.h"

namespace lvr {

class Window {
public:
  Window(int w, int h, std::string name);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  bool shouldClose() { return glfwWindowShouldClose(window); }

  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }

  void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

private:
  void initWindow();

  const uint32_t width;
  const uint32_t height;

  std::string windowName;
  GLFWwindow *window;

  // WaylandWindow waylandwindow{};

  VkWaylandSurfaceCreateInfoKHR vulkan_surface_create_info{};
};

} // namespace lvr
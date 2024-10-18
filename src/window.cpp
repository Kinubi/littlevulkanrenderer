#include "window.h"
#include "platform/waylandwindow.h"
#include <GLFW/glfw3.h>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>

namespace lvr {
Window::Window(int w, int h, std::string name)
    : width(w), height(h), windowName(name) {
  initWindow();
}

Window::~Window() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {

  //   vulkan_surface_create_info.sType =
  //       VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  //   vulkan_surface_create_info.display = waylandwindow.getDisplay();
  //   vulkan_surface_create_info.surface = waylandwindow.getSurface();

  //   VkResult result = vkCreateWaylandSurfaceKHR(
  //       instance, &vulkan_surface_create_info, NULL, surface);
  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, surface);
  if (result != VK_SUCCESS) {
    std::cout << result << std::endl;
    throw std::runtime_error("Failed to create window surface ");
  }
}
void Window::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window =
      glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

  glfwShowWindow(window);
}
} // namespace lvr
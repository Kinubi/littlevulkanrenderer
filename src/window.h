#pragma once

#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <wayland-client.h>

#include <string>

#include "vulkan/vulkan_wayland.h"

// #include "platform/waylandwindow.h"

namespace lvr {

class Window {
   public:
	Window(int32_t w, int32_t h, std::string name);
	~Window();

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	bool shouldClose() { return glfwWindowShouldClose(window); }

	VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
	bool wasWindowResized() { return framebufferResized; }
	void resetWindowResizedFlag() { framebufferResized = false; }
	GLFWwindow *getGLFWWindow() const { return window; }

	void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

   private:
	static void frameBufferResizeCallback(GLFWwindow *window, int32_t width, int32_t height);
	void initWindow();

	int32_t width;
	int32_t height;
	bool framebufferResized = false;

	std::string windowName;
	GLFWwindow *window;

	// WaylandWindow waylandwindow{};

	// VkWaylandSurfaceCreateInfoKHR vulkan_surface_create_info{};
};

}  // namespace lvr
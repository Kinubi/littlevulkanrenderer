#pragma once

#include "window.h"

// std lib headers

#include <vulkan/vulkan_core.h>

#include <optional>
#include <string>
#include <vector>

namespace lvr {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsAndComputeFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() { return graphicsAndComputeFamily.has_value() && presentFamily.has_value(); };
};

class Device {
   public:
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	Device(Window& window);
	~Device();

	// Not copyable or movable
	Device(const Device&) = delete;
	void operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;

	VkCommandPool getCommandPool() { return commandPool; }
	VkDevice device() { return Device_; }
	VkSurfaceKHR surface() { return surface_; }
	VkQueue graphicsQueue() { return graphicsQueue_; }
	VkQueue computeQueue() { return computeQueue_; }
	VkQueue presentQueue() { return presentQueue_; }

	SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
	VkFormat findSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);

	// Buffer Helper Functions
	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(
		VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

	void createImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory);

	void transitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t mipLevels,
		uint32_t layerCount);

	void generateMipmaps(
		VkImage image, VkFormat format, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

	VkPhysicalDeviceProperties properties;

	VkSampleCountFlagBits getMsaaSamples() { return msaaSamples; }

   private:
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	VkSampleCountFlagBits getMaxUsableSampleCount();

	// helper functions
	bool isDeviceSuitable(VkPhysicalDevice Device);
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice Device);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void hasGflwRequiredInstanceExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice Device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice Device);

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	Window& window;
	VkCommandPool commandPool;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkDevice Device_;
	VkSurfaceKHR surface_;
	VkQueue graphicsQueue_;
	VkQueue presentQueue_;
	VkQueue computeQueue_;

	const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	const std::vector<const char*> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

}  // namespace lvr
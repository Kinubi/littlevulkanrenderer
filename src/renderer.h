#pragma once

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdint>

#include "device.h"
#include "swapchain.h"
#include "window.h"

// std

#include <memory>
#include <vector>

namespace lvr {

class Renderer {
   public:
	Renderer(Window &window, Device &device);
	~Renderer();

	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;

	VkRenderPass getSwapChainRenderPass() const { return lvrSwapChain->getRenderPass(); }
	float getAspectRatio() const { return lvrSwapChain->extentAspectRatio(); }
	bool isFrameInProgress() const { return isFrameStarted; }

	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(isFrameStarted && "Cannot get command buffer when frame not in progress!");
		return commandBuffers[currentFrameIndex];
	}

	int32_t getFrameIndex() const {
		assert(isFrameStarted && "Cannot get frame index when frame is not in progress");
		return currentFrameIndex;
	}

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

   private:
	void createCommandBuffers();
	void createComputeCommandBuffers();
	void freeCommandBuffers();
	void recreateSwapChain();

	Window &lvrWindow;
	Device &lvrDevice;
	std::unique_ptr<SwapChain> lvrSwapChain;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandBuffer> computeCommandBuffers;

	uint32_t currentImageIndex;
	int32_t currentFrameIndex{0};
	bool isFrameStarted{false};
};

}  // namespace lvr

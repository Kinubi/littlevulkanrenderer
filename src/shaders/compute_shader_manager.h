#pragma once

#include <vulkan/vulkan.h>

#include <cassert>
#include <memory>
#include <vector>

#include "device.h"
#include "swapchain.h"
#include "window.h"

namespace lvr {
class ComputeShaderManager {
   public:
	ComputeShaderManager(Device &device);
	~ComputeShaderManager();

	ComputeShaderManager(const ComputeShaderManager &) = delete;
	ComputeShaderManager &operator=(const ComputeShaderManager &) = delete;

	bool getIsComputeDispatched() const { return isComputeDispatched; }

	VkCommandBuffer getCurrentComputeCommandBuffer() const {
		assert(isComputeDispatched && "Cannot get command buffer when frame not in progress!");
		return computeCommandBuffers[currentFrameIndex];
	}

	int32_t getFrameIndex() const {
		assert(isComputeDispatched && "Cannot get frame index when frame is not in progress");
		return currentFrameIndex;
	}
	VkCommandBuffer beginCompute();

	VkCommandBuffer endCompute();

   private:
	void createComputeCommandBuffers();
	void freeComputeCommandBuffers();

	Device &device;

	std::vector<VkCommandBuffer> computeCommandBuffers;

	int32_t currentFrameIndex{0};
	bool isComputeDispatched{false};
};
}  // namespace lvr
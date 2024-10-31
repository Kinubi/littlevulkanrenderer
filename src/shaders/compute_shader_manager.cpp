#include "compute_shader_manager.h"

namespace lvr {
ComputeShaderManager::ComputeShaderManager(Device& device) : device(device) {
	createComputeCommandBuffers();
}

ComputeShaderManager::~ComputeShaderManager() { freeComputeCommandBuffers(); }

VkCommandBuffer ComputeShaderManager::beginCompute() {
	assert(!isComputeDispatched && "Compute dispatch already in progress!");

	isComputeDispatched = true;
	VkCommandBuffer computeCommandBuffer = getCurrentComputeCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(computeCommandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	return computeCommandBuffer;
}

VkCommandBuffer ComputeShaderManager::endCompute() {
	assert(isComputeDispatched && "Compute dispatch not in progress!");
	auto computeCommandBuffer = getCurrentComputeCommandBuffer();
	if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
	isComputeDispatched = false;
	currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	return computeCommandBuffer;
}

void ComputeShaderManager::createComputeCommandBuffers() {
	computeCommandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(computeCommandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, computeCommandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void ComputeShaderManager::freeComputeCommandBuffers() {
	vkFreeCommandBuffers(
		device.device(),
		device.getCommandPool(),
		static_cast<uint32_t>(computeCommandBuffers.size()),
		computeCommandBuffers.data());
	computeCommandBuffers.clear();
}
}  // namespace lvr
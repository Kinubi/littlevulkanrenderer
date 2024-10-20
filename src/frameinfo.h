#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "camera.h"
#include "gameobject.h"
namespace lvr {
struct FrameInfo {
	int32_t frameIndex;
	float frameTime;

	VkCommandBuffer commandBuffer;
	Camera &camera;
	VkDescriptorSet globalDescriptorSet;

	GameObject::Map &gameObjects;
};
}  // namespace lvr
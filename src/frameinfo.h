#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "camera.h"
#include "gameobject.h"

namespace lvr {

#define MAX_LIGHTS 10

struct PointLight {
	glm::vec4 position{};
	glm::vec4 color{};
};

struct GlobalUbo {
	glm::mat4 projectionMatrix{1.0f};
	glm::mat4 viewMatrix{1.0f};
	glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f};
	PointLight pointLights[MAX_LIGHTS];
	int numLights;
};

struct FrameInfo {
	int32_t frameIndex;
	float frameTime;

	VkCommandBuffer commandBuffer;
	Camera &camera;
	VkDescriptorSet globalDescriptorSet;

	GameObject::Map &gameObjects;
};
}  // namespace lvr
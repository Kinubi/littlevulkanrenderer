#pragma once

#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>

#include "model.h"
#include "swapchain.h"
#include "textures/texture.h"

namespace lvr {

struct TransformComponent {
	glm::vec3 translation{};
	glm::vec3 scale{1.f, 1.f, 1.f};
	glm::vec3 rotation{};

	// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
	// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
	// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	glm::mat4 mat4();

	glm::mat3 normalMatrix();
};

struct PointLightComponent {
	float lightIntensity = 1.0f;
};

struct GameObjectBufferData {
	glm::mat4 modelMatrix{1.f};
	glm::mat4 normalMatrix{1.f};
};
class GameObjectManager;  // forward declare game object manager class

class GameObject {
   public:
	using id_t = uint32_t;
	using Map = std::unordered_map<id_t, GameObject>;

	GameObject(GameObject &&) = default;
	GameObject(const GameObject &) = delete;
	GameObject &operator=(const GameObject &) = delete;
	GameObject &operator=(GameObject &&) = delete;

	id_t getId() { return id; }

	VkDescriptorBufferInfo getBufferInfo(int32_t frameIndex);

	glm::vec4 color{};
	TransformComponent transform{};

	std::shared_ptr<Model> model{};
	std::shared_ptr<Texture> diffuseMap = nullptr;
	std::unique_ptr<PointLightComponent> pointLight = nullptr;

   private:
	GameObject(id_t objId, const GameObjectManager &mananger);

	id_t id;

	const GameObjectManager &gameObjectManager;
	friend class GameObjectManager;
};
class GameObjectManager {
   public:
	static constexpr int MAX_GAME_OBJECTS = 1000;
	GameObjectManager(Device &device);
	GameObjectManager(const GameObjectManager &) = delete;
	GameObjectManager &operator=(const GameObjectManager &) = delete;
	GameObjectManager(GameObjectManager &&) = delete;
	GameObjectManager &operator=(GameObjectManager &&) = delete;
	GameObject &createGameObject() {
		assert(currentId < MAX_GAME_OBJECTS && "Max game object count exceeded!");
		auto gameObject = GameObject{currentId++, *this};
		auto gameObjectId = gameObject.getId();
		gameObject.diffuseMap = textureDefault;
		gameObjects.emplace(gameObjectId, std::move(gameObject));
		return gameObjects.at(gameObjectId);
	}
	GameObject &makePointLight(
		float intensity = 10.f, float radius = 0.1f, glm::vec4 color = glm::vec4(1.f));
	VkDescriptorBufferInfo getBufferInfoForGameObject(
		int frameIndex, GameObject::id_t gameObjectId) const {
		return uboBuffers[frameIndex]->descriptorInfoForIndex(gameObjectId);
	}
	void updateBuffer(int frameIndex);
	GameObject::Map gameObjects{};
	std::vector<std::unique_ptr<Buffer>> uboBuffers{SwapChain::MAX_FRAMES_IN_FLIGHT};

   private:
	GameObject::id_t currentId = 0;
	std::shared_ptr<Texture> textureDefault;
};

}  // namespace lvr
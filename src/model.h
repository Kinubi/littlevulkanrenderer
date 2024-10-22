#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "buffer.h"
#include "device.h"

namespace lvr {

class Model {
   public:
	struct Vertex {
		glm::vec3 position{};
		glm::vec4 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		bool operator==(const Vertex &other) const {
			return position == other.position && color == other.color && normal == other.normal &&
				   uv == other.uv;
		}
	};

	struct Builder {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		void loadModel(const std::string &filepath);
	};

	Model(Device &device, const Builder &builder);
	~Model();

	Model(const Model &) = delete;
	void operator=(const Model &) = delete;

	static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filepath);

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

   private:
	void createVertexBuffers(const std::vector<Vertex> &vertices);
	void createIndexBuffers(const std::vector<uint32_t> &indices);

	Device &lvrDevice;

	std::unique_ptr<Buffer> vertexBuffer;
	uint32_t vertexCount;

	bool hasIndexBuffer = false;
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;
};

}  // namespace lvr
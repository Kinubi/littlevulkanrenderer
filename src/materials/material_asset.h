#pragma once

#include <memory>

#include "glm/glm.hpp"

namespace lvr {

struct Material {
	glm::vec3 albedo{1};
	glm::vec3 emissiveColor{1};
	float emissiveStrength = 0;
	float roughness = 0.5;
	float metalness = 0.5;
	float refractiveIndex = 1;
};

class MaterialAsset {
   public:
	MaterialAsset() { material = std::make_shared<Material>(); };
	~MaterialAsset() {};

	bool operator==(const MaterialAsset& second) const {
		return this->getAlbedo() == second.getAlbedo() &&
			   this->getEmissiveColor() == second.getEmissiveColor() &&
			   this->getEmissiveStrength() == second.getEmissiveStrength() &&
			   this->getRoughness() == second.getRoughness() &&
			   this->getMetalness() == second.getMetalness() &&
			   this->getRefractiveIndex() == second.getRefractiveIndex();
	}
	const glm::vec3 getAlbedo() const { return material->albedo; }
	const glm::vec3 getEmissiveColor() const { return material->emissiveColor; }
	const float getEmissiveStrength() const { return material->emissiveStrength; }
	const float getRoughness() const { return material->roughness; }
	const float getMetalness() const { return material->metalness; }
	const float getRefractiveIndex() const { return material->refractiveIndex; }
	const std::shared_ptr<Material> getMaterial() const { return material; }
	const id_t getID() const { return id; }

	void setAlbedo(const glm::vec3& albedo) { material->albedo = albedo; }
	void setEmissiveColor(const glm::vec3& emissiveColor) {
		material->emissiveColor = emissiveColor;
	}
	void setEmissiveStrength(float emissiveStrength) {
		material->emissiveStrength = emissiveStrength;
	}
	void setRoughness(float roughness) { material->roughness = roughness; }
	void setMetalness(float metalness) { material->metalness = metalness; }
	void setRefractiveIndex(float refractiveIndex) { material->refractiveIndex = refractiveIndex; }
	void setID(id_t id) { this->id = id; }

   private:
	std::shared_ptr<Material> material{};
	id_t id{0};
};

}  // namespace lvr

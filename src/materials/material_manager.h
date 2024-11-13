#pragma once

#include <map>
#include <vector>

#include "device.h"
#include "material_asset.h"

namespace lvr {
class MaterialManager {
   public:
	MaterialManager(Device &device) {};
	~MaterialManager() {};
	MaterialManager(const MaterialManager &) = delete;
	MaterialManager &operator=(const MaterialManager &) = delete;
	MaterialManager(MaterialManager &&) = delete;
	MaterialManager &operator=(MaterialManager &&) = delete;
	std::map<id_t, MaterialAsset> materialAssets{};
	std::vector<Material> getMaterials() {
		std::vector<Material> materials;
		for (auto &materialAsset : materialAssets) {
			materials.emplace_back(materialAsset.second.getMaterial());
		}
		return materials;
	}

	const id_t &create(MaterialAsset &materialAsset) {
		for (auto &material : materialAssets) {
			if (material.second == materialAsset) {
				return material.first;
			}
		}
		materialAsset.setID(currentId++);
		return materialAssets.emplace(materialAsset.getID(), std::move(materialAsset)).first->first;
	}

   private:
	id_t currentId = 0;
};
}  // namespace lvr
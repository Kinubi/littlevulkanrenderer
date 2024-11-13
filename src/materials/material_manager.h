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
	std::map<id_t, MaterialAsset> materials{};

	const id_t &create(MaterialAsset &materialAsset) {
		for (auto &material : materials) {
			if (material.second == materialAsset) {
				return material.first;
			}
		}
		materialAsset.setID(currentId++);
		materials.emplace(currentId, std::move(materialAsset));
		return materials.emplace(currentId, std::move(materialAsset)).first->first;
	}

   private:
	id_t currentId = 0;
};
}  // namespace lvr
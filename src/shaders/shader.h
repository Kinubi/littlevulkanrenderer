#pragma once

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../device.h"

namespace lvr {

struct ShaderInfo {
	ShaderInfo() = default;
	VkShaderModule shaderModule{};
	VkShaderModuleCreateInfo createInfo{};
	VkPipelineShaderStageCreateInfo shaderCreateInfo{};
};

struct Source {
	VkShaderStageFlagBits shaderBitFlags;
	std::string source{};
};

class Shader {
   public:
	Shader(const std::string& filePath);
	Shader(const std::vector<std::string> filePaths);
	~Shader();

	static void Create(
		Device& device,
		const std::vector<std::string> filePaths,
		std::vector<ShaderInfo>& shaderStages);
	static void Create(Device& device, const std::string& filePath, ShaderInfo& shaderStage);

   private:
	std::string ReadFile(const std::string& filePath);
	Source PreProcess(const std::string& source, const std::string& extension);

	void CompileOrGetVulkanBinaries(
		std::vector<Source>& shaderSources, const std::vector<std::string> filePaths);
	void Reflect(VkShaderStageFlagBits stage, const std::vector<uint32_t>& shaderData);

   private:
	std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> m_VulkanSPIRV{};
};

}  // namespace lvr
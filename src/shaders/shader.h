#pragma once

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../device.h"

namespace lvr {

struct ShaderInfo {
	ShaderInfo() = default;
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo createInfo{};
	VkPipelineShaderStageCreateInfo shaderCreateInfo{};
};

struct Source {
	VkShaderStageFlagBits shaderBitFlags;
	std::string sourceString{};
	std::filesystem::path filePath;
	std::vector<uint32_t> m_VulkanSPIRV;
};
class Shader {
   public:
	Shader(Device& device, const std::string& filePath);

	~Shader();

	static std::vector<std::unique_ptr<Shader>> Create(
		Device& device, const std::vector<std::string> filePaths);
	static std::unique_ptr<Shader> Create(Device& device, const std::string& filePath);

	const Source getSource() const { return source; }
	const ShaderInfo getShaderInfo() const { return shaderInfo; }

   private:
	std::string ReadFile(const std::string& filePath);
	void PreProcess(Source& source);

	void CompileOrGetVulkanBinaries(Source& source);
	void Reflect(Source& source);

   private:
	Device& device;
	Source source{};

	ShaderInfo shaderInfo{};
};

}  // namespace lvr
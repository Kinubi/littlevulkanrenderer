#include "shader.h"

#include <shaderc/shaderc.h>
#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace lvr {

namespace Utils {
static VkShaderStageFlagBits ShaderTypeFromString(const std::string& type) {
	if (type == ".vert") return VK_SHADER_STAGE_VERTEX_BIT;
	if (type == ".frag" || type == ".pixel") return VK_SHADER_STAGE_FRAGMENT_BIT;
	if (type == ".comp") return VK_SHADER_STAGE_COMPUTE_BIT;

	return (VkShaderStageFlagBits)0;
}

static shaderc_shader_kind ShaderStageToShaderC(VkShaderStageFlagBits stage) {
	switch (stage) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			return shaderc_glsl_vertex_shader;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return shaderc_glsl_fragment_shader;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return shaderc_glsl_compute_shader;
	}
	return (shaderc_shader_kind)0;
}

static const char* ShaderStageToString(VkShaderStageFlagBits stage) {
	switch (stage) {
		case VK_SHADER_STAGE_VERTEX_BIT:
			return "VULKAN_VERTEX_SHADER";
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return "VULKAN_FRAGMENT_SHADER";
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return "VULKAN_COMPUTE_SHADER";
	}
	return nullptr;
}

static const char* GetCacheDirectory() {
	// TODO: make sure the assets directory is valid
	return "shaders/cache/";
}

static void CreateCacheDirectoryIfNeeded() {
	std::string cacheDirectory(GetCacheDirectory());
	if (!std::filesystem::exists(cacheDirectory))
		std::filesystem::create_directories(cacheDirectory);
}
}  // namespace Utils

Shader::Shader(const std::string& filePath) {
	Utils::CreateCacheDirectoryIfNeeded();
	std::vector<Source> sources{};
	std::string source = ReadFile(filePath);

	std::filesystem::path filepath = std::filesystem::path(filePath);
	sources.push_back(PreProcess(source, filepath.extension()));

	CompileOrGetVulkanBinaries(sources, std::vector<std::string>{filePath});
}

Shader::Shader(const std::vector<std::string> filePaths) {
	std::vector<Source> sources{};
	Utils::CreateCacheDirectoryIfNeeded();
	for (std::string filePath : filePaths) {
		std::string source = ReadFile(filePath);

		std::filesystem::path filepath = std::filesystem::path(filePath);
		sources.push_back(PreProcess(source, filepath.extension()));
	}
	CompileOrGetVulkanBinaries(sources, filePaths);
}

Shader::~Shader() {}

std::string Shader::ReadFile(const std::string& filepath) {
	std::string result{};
	std::ifstream in(
		filepath,
		std::ios::in | std::ios::binary);  // ifstream closes itself due to RAII
	if (in) {
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		if (size != -1) {
			result.resize(size);
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
		} else {
			throw std::runtime_error("Could not read from file");
		}
	} else {
		throw std::runtime_error("Could not open file");
	}

	return result;
}

Source Shader::PreProcess(const std::string& source, const std::string& extension) {
	Source shaderSource{};

	assert(Utils::ShaderTypeFromString(extension) && "Invalid shader type specified");
	shaderSource.shaderBitFlags = Utils::ShaderTypeFromString(extension);
	shaderSource.source = source;

	return shaderSource;
}

void Shader::CompileOrGetVulkanBinaries(
	std::vector<Source>& shaderSources, const std::vector<std::string> filePaths) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	const bool optimize = true;
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance);

	std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

	auto& shaderData = m_VulkanSPIRV;
	shaderData.clear();
	int32_t i = 0;
	for (auto&& [stage, source] : shaderSources) {
		std::filesystem::path shaderFilePath = filePaths[i];
		std::filesystem::path cachedPath =
			cacheDirectory / (shaderFilePath.filename().string() + ".spv");

		std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
		if (in.is_open()) {
			in.seekg(0, std::ios::end);
			auto size = in.tellg();
			in.seekg(0, std::ios::beg);

			auto& data = shaderData[stage];
			data.resize(size / sizeof(uint32_t));
			in.read((char*)data.data(), size);
		} else {
			shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
				source,
				Utils::ShaderStageToShaderC(stage),
				filePaths[i].c_str(),
				options);
			if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
				throw std::runtime_error(module.GetErrorMessage());
				assert_perror(false);
			}

			shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

			std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
			if (out.is_open()) {
				auto& data = shaderData[stage];
				out.write((char*)data.data(), data.size() * sizeof(uint32_t));
				out.flush();
				out.close();
			}
		}
		i++;
	}

	for (auto&& [stage, data] : shaderData) Reflect(stage, data);
}

void Shader::Reflect(VkShaderStageFlagBits stage, const std::vector<uint32_t>& shaderData) {
	spirv_cross::Compiler compiler(shaderData);
	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	for (const auto& resource : resources.uniform_buffers) {
		const auto& bufferType = compiler.get_type(resource.base_type_id);
		uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
		int memberCount = bufferType.member_types.size();
	}
}

void Shader::Create(Device& device, const std::string& filepath, ShaderInfo& shaderStage) {
	Shader shaders = Shader(filepath);
	int32_t i = 0;
	auto shader = shaders.m_VulkanSPIRV.begin();

	shaderStage.createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderStage.createInfo.codeSize = 4 * shader->second.size();
	shaderStage.createInfo.pCode = reinterpret_cast<const uint32_t*>(shader->second.data());

	if (vkCreateShaderModule(
			device.device(),
			&shaderStage.createInfo,
			nullptr,
			&shaderStage.shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader");
	}

	shaderStage.shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.shaderCreateInfo.stage = shader->first;
	shaderStage.shaderCreateInfo.module = shaderStage.shaderModule;
	shaderStage.shaderCreateInfo.pName = "main";
	shaderStage.shaderCreateInfo.flags = 0;
	shaderStage.shaderCreateInfo.pNext = nullptr;
	i++;
}

void Shader::Create(
	Device& device,
	const std::vector<std::string> filePaths,
	std::vector<ShaderInfo>& shaderStages) {
	Shader shaders = Shader(filePaths);
	int32_t i = 0;
	for (auto shader : shaders.m_VulkanSPIRV) {
		shaderStages[i].createInfo.codeSize = 4 * shader.second.size();
		shaderStages[i].createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderStages[i].createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.second.data());

		if (vkCreateShaderModule(
				device.device(),
				&shaderStages[i].createInfo,
				nullptr,
				&shaderStages[i].shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader");
		}

		shaderStages[i].shaderCreateInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[i].shaderCreateInfo.stage = shader.first;
		shaderStages[i].shaderCreateInfo.module = shaderStages[i].shaderModule;
		shaderStages[i].shaderCreateInfo.pName = "main";
		shaderStages[i].shaderCreateInfo.flags = 0;
		shaderStages[i].shaderCreateInfo.pNext = nullptr;

		i++;
	}
}

}  // namespace lvr
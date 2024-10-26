#include "shader.h"

#include <shaderc/shaderc.h>
#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstdint>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <iterator>
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

Shader::Shader(Device& device, const std::string& filePath) : device(device) {
	Utils::CreateCacheDirectoryIfNeeded();
	source.filePath = filePath;

	std::filesystem::path filepath = std::filesystem::path(filePath);
	PreProcess(source);

	CompileOrGetVulkanBinaries(source);
}

Shader::~Shader() { vkDestroyShaderModule(device.device(), shaderInfo.shaderModule, nullptr); }

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

void Shader::PreProcess(Source& source) {
	assert(
		Utils::ShaderTypeFromString(source.filePath.extension()) &&
		"Invalid shader type specified");
	source.shaderBitFlags = Utils::ShaderTypeFromString(source.filePath.extension());
	source.sourceString = ReadFile(source.filePath);
}

void Shader::CompileOrGetVulkanBinaries(Source& shaderSource) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	const bool optimize = true;
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_performance);

	std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

	auto& shaderData = source.m_VulkanSPIRV;
	shaderData.clear();

	std::filesystem::path shaderFilePath = source.filePath;
	std::filesystem::path cachedPath =
		cacheDirectory / (shaderFilePath.filename().string() + ".spv");

	std::ifstream in{cachedPath, std::ios::ate | std::ios::binary};
	if (in.is_open()) {
		size_t size = static_cast<size_t>(in.tellg());

		in.seekg(0);
		auto& data = source.m_VulkanSPIRV;
		data.resize(size / sizeof(uint32_t));
		in.read((char*)data.data(), size);
		in.close();
	} else {
		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
			source.sourceString,
			Utils::ShaderStageToShaderC(source.shaderBitFlags),
			source.filePath.c_str(),
			options);
		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			throw std::runtime_error(module.GetErrorMessage());
			assert_perror(false);
		}

		source.m_VulkanSPIRV = std::vector<uint32_t>(module.cbegin(), module.cend());

		std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
		if (out.is_open()) {
			auto& data = source.m_VulkanSPIRV;
			out.write((char*)data.data(), data.size() * sizeof(uint32_t));
			out.flush();
			out.close();
		}
	}

	Reflect(source);
}

void Shader::Reflect(Source& source) {
	spirv_cross::Compiler compiler(source.m_VulkanSPIRV);
	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	for (const auto& resource : resources.uniform_buffers) {
		const auto& bufferType = compiler.get_type(resource.base_type_id);
		uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
		uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
		int memberCount = bufferType.member_types.size();
	}
}

std::unique_ptr<Shader> Shader::Create(Device& device, const std::string& filepath) {
	std::unique_ptr<Shader> shader = std::make_unique<Shader>(Shader(device, filepath));

	shader->shaderInfo.createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader->shaderInfo.createInfo.codeSize = 4 * shader->source.m_VulkanSPIRV.size();
	shader->shaderInfo.createInfo.pCode =
		reinterpret_cast<const uint32_t*>(shader->source.m_VulkanSPIRV.data());

	if (vkCreateShaderModule(
			device.device(),
			&shader->shaderInfo.createInfo,
			nullptr,
			&shader->shaderInfo.shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader");
	}

	shader->shaderInfo.shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader->shaderInfo.shaderCreateInfo.stage = shader->source.shaderBitFlags;
	shader->shaderInfo.shaderCreateInfo.module = shader->shaderInfo.shaderModule;
	shader->shaderInfo.shaderCreateInfo.pName = "main";
	shader->shaderInfo.shaderCreateInfo.flags = 0;
	shader->shaderInfo.shaderCreateInfo.pNext = VK_NULL_HANDLE;

	return shader;
}

std::vector<std::unique_ptr<Shader>> Shader::Create(
	Device& device, const std::vector<std::string> filePaths) {
	std::vector<std::unique_ptr<Shader>> shaders;
	int32_t i = 0;
	for (auto filePath : filePaths) {
		shaders.push_back(Create(device, filePath));
	}
	// std::cout << shaders[1]->shaderInfo.shaderCreateInfo.pNext << std::endl;
	return shaders;
}

}  // namespace lvr
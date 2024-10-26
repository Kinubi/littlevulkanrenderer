#pragma once

#include <vulkan/vulkan_core.h>

#include <memory>
#include <string>
#include <vector>

#include "device.h"
#include "shaders/shader.h"

namespace lvr {

struct PipelineConfigInfo {
	PipelineConfigInfo() = default;
	PipelineConfigInfo(const PipelineConfigInfo &) = delete;
	PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};
class Pipeline {
   public:
	Pipeline(
		Device &device,
		const std::vector<std::string> filePaths,
		const PipelineConfigInfo &configInfo);

	~Pipeline();

	Pipeline(const Pipeline &) = delete;
	Pipeline operator=(const Pipeline &) = delete;

	void bind(VkCommandBuffer commandBuffer);

	static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);
	static void enableAlphaBlending(PipelineConfigInfo &configInfo);

   private:
	void createGraphicsPipeline(
		const std::vector<std::string> filePaths, const PipelineConfigInfo &configInfo);

	void createShaders(const std::string &fragFilepath, const std::string &vertFilepath);

	Device &device;
	VkPipeline graphicsPipeline;

	std::vector<std::unique_ptr<Shader>> shaders{};
	VkPipelineShaderStageCreateInfo createInfos[];
};
}  // namespace lvr
#include "application.h"
#include "device.h"
#include "pipeline.h"
#include "window.h"
#include <GLFW/glfw3.h>

#include <cstdint>
#include <vulkan/vulkan_core.h>

// std
#include <array>
#include <memory>
#include <stdexcept>
namespace lvr {

Application::Application() {
  createPipelineLayout();
  createPipeline();
  createCommandBuffers();
}

Application::~Application() {
  vkDestroyPipelineLayout(lvrDevice.device(), pipelineLayout, nullptr);
}

void Application::OnStart() {

  while (!lvrWIndow.shouldClose()) {

    OnUpdate();
  }

  vkDeviceWaitIdle(lvrDevice.device());
}

void Application::OnUpdate() {
  glfwPollEvents();
  drawFrame();
  // std::cout << "Updating" << std::endl;
}

void Application::createPipelineLayout() {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(lvrDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout");
  }
}

void Application::createPipeline() {
  auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(
      lvrSwapChain.width(), lvrSwapChain.height());
  pipelineConfig.renderPass = lvrSwapChain.getRenderPass();
  pipelineConfig.pipelineLayout = pipelineLayout;
  lvrPipeline = std::make_unique<Pipeline>(
      lvrDevice, "shaders/simple_shader.vert.spv",
      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void Application::createCommandBuffers() {
  commandBuffers.resize(lvrSwapChain.imageCount());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = lvrDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(lvrDevice.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }

  for (int i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("Failed to begin Command Buffer recording!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = lvrSwapChain.getRenderPass();
    renderPassInfo.framebuffer = lvrSwapChain.getFrameBuffer(i);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = lvrSwapChain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, .1f, .1f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    lvrPipeline->bind(commandBuffers[i]);
    vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffers[i]);

    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to end Command Buffer recording!");
    }
  }
}
void Application::drawFrame() {
  uint32_t imageIndex;
  auto result = lvrSwapChain.acquireNextImage(&imageIndex);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed ot acquire swap chain image!");
  }

  result = lvrSwapChain.submitCommandBuffers(&commandBuffers[imageIndex],
                                             &imageIndex);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image!");
  }
};
} // namespace lvr
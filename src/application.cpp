#include "application.h"
#include "device.h"
#include "model.h"
#include "pipeline.h"
#include "swapchain.h"
#include "window.h"
#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

// std
#include <array>
#include <memory>
#include <stdexcept>

namespace lvr {

struct SimplePushConstantData {
  glm::vec2 offset;
  alignas(16) glm::vec4 color;
};

Application::Application() {
  loadModels();
  createPipelineLayout();
  recreateSwapChain();
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

void Application::loadModels() {
  std::vector<Model::Vertex> vertices{
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 0.5f}}};

  lvrModel = std::make_unique<Model>(lvrDevice, vertices);
}

void Application::createPipelineLayout() {

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(lvrDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout");
  }
}

void Application::createPipeline() {
  assert(lvrSwapChain != nullptr && "Cannot create pipeline before swap chain");
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  Pipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = lvrSwapChain->getRenderPass();
  pipelineConfig.pipelineLayout = pipelineLayout;
  lvrPipeline = std::make_unique<Pipeline>(
      lvrDevice, "shaders/simple_shader.vert.spv",
      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void Application::recreateSwapChain() {
  auto extent = lvrWIndow.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = lvrWIndow.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(lvrDevice.device());

  if (lvrSwapChain == nullptr) {

    lvrSwapChain = std::make_unique<SwapChain>(lvrDevice, extent);
  } else {
    lvrSwapChain =
        std::make_unique<SwapChain>(lvrDevice, extent, std::move(lvrSwapChain));
    if (lvrSwapChain->imageCount() != commandBuffers.size()) {
      freeCommandBuffers();
      createCommandBuffers();
    }
  }

  createPipeline();
}

void Application::createCommandBuffers() {
  commandBuffers.resize(lvrSwapChain->imageCount());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = lvrDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(lvrDevice.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }
}

void Application::freeCommandBuffers() {
  vkFreeCommandBuffers(lvrDevice.device(), lvrDevice.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  commandBuffers.clear();
}

void Application::drawFrame() {
  uint32_t imageIndex;
  auto result = lvrSwapChain->acquireNextImage(&imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed ot acquire swap chain image!");
  }

  recordCommandBuffer(imageIndex);
  result = lvrSwapChain->submitCommandBuffers(&commandBuffers[imageIndex],
                                              &imageIndex);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ||
      lvrWIndow.wasWindowResized()) {
    lvrWIndow.resetWindowResizedFlag();
    recreateSwapChain();
    return;
  }
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image!");
  }
}

void Application::recordCommandBuffer(uint32_t imageIndex) {
  static int32_t frame = 0;
  frame = (frame + 1) % 1000;
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to begin Command Buffer recording!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = lvrSwapChain->getRenderPass();
  renderPassInfo.framebuffer = lvrSwapChain->getFrameBuffer(imageIndex);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = lvrSwapChain->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(lvrSwapChain->getSwapChainExtent().width);
  viewport.height =
      static_cast<float>(lvrSwapChain->getSwapChainExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0}, lvrSwapChain->getSwapChainExtent()};
  vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
  vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

  lvrPipeline->bind(commandBuffers[imageIndex]);
  lvrModel->bind(commandBuffers[imageIndex]);

  for (int32_t j = 0; j < 4; j++) {
    SimplePushConstantData push{};
    push.offset = {-0.5f + frame * 0.002f, -0.4f + j * 0.25f};
    push.color = {0.0f, 0.0f, 0.2f + 0.2f * j, 1.0f};

    vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);

    lvrModel->draw(commandBuffers[imageIndex]);
  }

  vkCmdEndRenderPass(commandBuffers[imageIndex]);

  if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
    throw std::runtime_error("Failed to end Command Buffer recording!");
  }
}
} // namespace lvr
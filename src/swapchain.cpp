#include "swapchain.h"

#include "device.h"

// std
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace lvr {

SwapChain::SwapChain(Device& deviceRef, VkExtent2D extent)
	: device{deviceRef}, windowExtent{extent} {
	init();
}

SwapChain::SwapChain(Device& deviceRef, VkExtent2D extent, std::shared_ptr<SwapChain> previous)
	: device{deviceRef}, windowExtent{extent}, oldSwapchain(previous) {
	init();

	// clean up old swap chain
	oldSwapchain = nullptr;
}

void SwapChain::init() {
	createSwapChain();
	createImageViews();
	createRenderPass();
	createDepthResources();
	createColorResources();
	createFramebuffers();
	createSyncObjects();
}

SwapChain::~SwapChain() {
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device.device(), imageView, nullptr);
	}
	swapChainImageViews.clear();

	if (swapChain != nullptr) {
		vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
		swapChain = nullptr;
	}

	for (int i = 0; i < depthImages.size(); i++) {
		vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
		vkDestroyImageView(device.device(), colorImageViews[i], nullptr);
		vkDestroyImage(device.device(), depthImages[i], nullptr);
		vkDestroyImage(device.device(), colorImages[i], nullptr);
		vkFreeMemory(device.device(), depthImageMemorys[i], nullptr);
		vkFreeMemory(device.device(), colorImageMemorys[i], nullptr);
	}

	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(device.device(), renderPass, nullptr);

	// cleanup synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device.device(), renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device.device(), imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(device.device(), computeFinishedSemaphores[i], nullptr);
		if (inFlightFences[i] != VK_NULL_HANDLE)
			vkDestroyFence(device.device(), inFlightFences[i], nullptr);
		if (computeInFlightFences[i] != VK_NULL_HANDLE)
			vkDestroyFence(device.device(), computeInFlightFences[i], nullptr);
	}
}

VkResult SwapChain::acquireNextImage(uint32_t* imageIndex) {
	vkWaitForFences(
		device.device(),
		1,
		&inFlightFences[currentFrame],
		VK_TRUE,
		std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(
		device.device(),
		swapChain,
		std::numeric_limits<uint64_t>::max(),
		imageAvailableSemaphores[currentFrame],	 // must be a not signaled
		// semaphore
		VK_NULL_HANDLE,
		imageIndex);

	return result;
}

VkResult SwapChain::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex) {
	if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {
		computeFinishedSemaphores[currentFrame],
		imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 2;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = buffers;

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device.device(), 1, &inFlightFences[currentFrame]);
	if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;

	auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}

void SwapChain::submitComputeCommandBuffers(const VkCommandBuffer* buffers) {
	if (computeInFlightFences[currentFrame] != VK_NULL_HANDLE) {
		vkWaitForFences(
			device.device(),
			1,
			&computeInFlightFences[currentFrame],
			VK_TRUE,
			UINT64_MAX);
	}

	vkResetFences(device.device(), 1, &computeInFlightFences[currentFrame]);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = buffers;

	VkSemaphore signalSemaphores[] = {computeFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(device.computeQueue(), 1, &submitInfo, computeInFlightFences[currentFrame]) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

void SwapChain::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = device.surface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
	uint32_t queueFamilyIndices[] = {
		indices.graphicsAndComputeFamily.value(),
		indices.presentFamily.value()};

	if (indices.graphicsAndComputeFamily.value() != indices.presentFamily.value()) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;	   // Optional
		createInfo.pQueueFamilyIndices = nullptr;  // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->swapChain;

	if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	// we only specified a minimum number of images in the swap chain, so the
	// implementation is allowed to create a swap chain with more. That's why
	// we'll first query the final number of images with vkGetSwapchainImagesKHR,
	// then resize the container and finally call it again to retrieve the
	// handles.
	vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void SwapChain::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapChainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = swapChainImageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = getSwapChainImageFormat();
	colorAttachment.samples = device.getMsaaSamples();
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = device.getMsaaSamples();
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 3> attachments = {
		colorAttachment,
		depthAttachment,
		colorAttachmentResolve};
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void SwapChain::createFramebuffers() {
	swapChainFramebuffers.resize(imageCount());
	for (size_t i = 0; i < imageCount(); i++) {
		std::array<VkImageView, 3> attachments = {
			colorImageViews[i],
			depthImageViews[i],
			swapChainImageViews[i]};

		VkExtent2D swapChainExtent = getSwapChainExtent();
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(
				device.device(),
				&framebufferInfo,
				nullptr,
				&swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void SwapChain::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	swapChainDepthFormat = depthFormat;
	VkExtent2D swapChainExtent = getSwapChainExtent();

	depthImages.resize(imageCount());
	depthImageMemorys.resize(imageCount());
	depthImageViews.resize(imageCount());

	for (int i = 0; i < depthImages.size(); i++) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = swapChainExtent.width;
		imageInfo.extent.height = swapChainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = device.getMsaaSamples();
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;

		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImages[i],
			depthImageMemorys[i]);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::createColorResources() {
	VkFormat colorFormat = getSwapChainImageFormat();
	VkExtent2D swapChainExtent = getSwapChainExtent();

	colorImages.resize(imageCount());
	colorImageMemorys.resize(imageCount());
	colorImageViews.resize(imageCount());

	for (int i = 0; i < colorImages.size(); i++) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = swapChainExtent.width;
		imageInfo.extent.height = swapChainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = colorFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage =
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageInfo.samples = device.getMsaaSamples();
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;

		device.createImageWithInfo(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			colorImages[i],
			colorImageMemorys[i]);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = colorImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = colorFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device.device(), &viewInfo, nullptr, &colorImageViews[i]) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(
				device.device(),
				&semaphoreInfo,
				nullptr,
				&imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(
				device.device(),
				&semaphoreInfo,
				nullptr,
				&renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		if (vkCreateSemaphore(
				device.device(),
				&semaphoreInfo,
				nullptr,
				&computeFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device.device(), &fenceInfo, nullptr, &computeInFlightFences[i]) !=
				VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create compute synchronization objects for a frame!");
		}
	}
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		std::cout << availableFormat.colorSpace << std::endl;
		if (availableFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 &&
			availableFormat.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT)
			return availableFormat;
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}
	return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(
	const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		std::cout << availablePresentMode << std::endl;
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			std::cout << "Present mode: Mailbox" << std::endl;
			return availablePresentMode;
		}
	}

	// for (const auto &availablePresentMode : availablePresentModes) {
	//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
	//     std::cout << "Present mode: Immediate" << std::endl;
	//     return availablePresentMode;
	//   }
	// }

	std::cout << "Present mode: V-Sync" << std::endl;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = windowExtent;
		actualExtent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkFormat SwapChain::findDepthFormat() {
	return device.findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

}  // namespace lvr

#include "managers.h"
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <limits>

void swapchain_manager::create_swapchain(uint32_t graphics_family_index, uint32_t present_family_index) {
	auto caps = m_phys_device.getSurfaceCapabilitiesKHR(*m_surface);

	vk::Extent2D extent;
	if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = caps.currentExtent;
	} else {
		int pixel_width, pixel_height;
		glfwGetFramebufferSize(m_window, &pixel_width, &pixel_height);
		if (pixel_width <= 0 || pixel_height <= 0) {
			spdlog::error("Bad window size: w: {}, h: {} ", pixel_width, pixel_height);
		}

		uint32_t width = static_cast<uint32_t>(pixel_width), 
				 height = static_cast<uint32_t>(pixel_height);
		extent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
		extent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
	}

	auto formats = m_phys_device.getSurfaceFormatsKHR(*m_surface);
	vk::SurfaceFormatKHR selected_format = formats[0]; // 默认保底
	for (const auto& f : formats) {
		if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) { // use the sRGB color format
			selected_format = f;
			break;
		}
	}

	auto present_modes = m_phys_device.getSurfacePresentModeKHR(*m_surface);
	vk::PresentModeKHR selected_present_mode = vk::PresentModeKHR::eFifo;

	for (const auto& pm : present_modes) {
		if (pm == vk::PresentModeKHR::eMailbox) {
			selected_present_mode = pm;
		}
	}

	uint32_t queue_family_indices[] = { graphics_family_index, present_family_index };

	// create swapchian
	vk::SwapchainCreateInfoKHR create_info(
			{},
			*m_surface,
			caps.minImageCount +1,
			selected_format.format,
			selected_format.colorSpace,
			extent,
			1,
			vk::ImageUseFlagBits::eColorAttachment
			);
	if (graphics_family_index != present_family_index) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queue_family_indices;
	} else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}
	createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBits::eOpaque;
    createInfo.presentMode = selected_present_mode;
    createInfo.clipped = VK_TRUE;
};

void swapchain_manager::cleanup_swapchain() {
	if (m_swapchain == nullptr) {
		spdlog::warning("The swapchain is null. ");
		return;
	}
}

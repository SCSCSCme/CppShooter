#include "managers.h"
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <limits>

void swapchain_manager::create_swapchain() {
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
		width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
		height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
	}
};

void swapchain_manager::cleanup_swapchain() {
}

#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include <spdlog/spdlog.h>

enum class swapchain_status {
	Success,
    OutOfDate, // 必须重建
    Suboptimal // 还可以用，但建议重建
};

class swapchain_manager {
	public:
		swapchain_manager(vk::raii::Device& device, 
				vk::raii::PhysicalDevice& physical_device,
				vk::raii::SurfaceKHR& surface,
				GLFWwindow* window
				queue_family_indices& indices
				)
			: m_device(device),
			m_phys_device(physical_device),
			m_surface(surface),
			m_window(window) {
				spdlog::info("Swapchain manager create. ");
		}

		void recreate(uint32_t graphics_family_index, uint32_t present_family_index);

	private:
		void cleanup_swapchain(uint32_t graphics_family_index, uint32_t present_family_index); // it will use the param from the recreate function
		void create_swapchain();

	private:
		vk::raii::SwapchainKHR swapchain = nullptr;
		swapchain_status status;

		vk::raii::Device& m_device;
		vk::raii::PhysicalDevice& m_phys_device;
		vk::raii::SurfaceKHR& m_surface;
		GLFWwindow* m_window = nullptr;
};

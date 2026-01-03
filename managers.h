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
				)
			: m_device(device),
			m_phys_device(physical_device),
			m_surface(surface),
			m_window(window) {
				spdlog::info("Swapchain manager create. ");
		}

		void recreate();

	private:
		void cleanup_swapchain();
		void create_swapchain();

	private:
		vk::raii::SwapchainKHR swapchain = nullptr;
		swapchain_status status;

		vk::raii::Device& m_device;
		vk::raii::PhysicalDevice& m_phys_device;
		vk::raii::SurfaceKHR& m_surface;
		GLFWwindow* m_window = nullptr;
};

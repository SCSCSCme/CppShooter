#include "managers.h"
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <limits>
#include <vector>

swapchain_manager::swapchain_manager(vk::raii::Device* device,
                                     vk::raii::PhysicalDevice* physical_device,
                                     vk::raii::SurfaceKHR* surface,
                                     GLFWwindow* window
                                     ) {
    spdlog::info("Swapchain manager create");
    m_device = device;
    m_phys_device = physical_device;
    m_surface = static_cast<vk::SurfaceKHR>(**surface);
}

void swapchain_manager::recreate(uint32_t graphics_family_index, uint32_t present_family_index) {
    cleanup_swapchain();
    create_swapchain(graphics_family_index, present_family_index);
}

void swapchain_manager::create_swapchain(uint32_t graphics_family_index, uint32_t present_family_index) {
    auto caps = m_phys_device->getSurfaceCapabilitiesKHR(m_surface);

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

    auto formats = m_phys_device->getSurfaceFormatsKHR(m_surface);
    vk::SurfaceFormatKHR selected_format = formats[0]; // 默认保底
    for (const auto& f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) { // use the sRGB color format
            selected_format = f;
            break;
        }
    }

    auto present_modes = m_phys_device->getSurfacePresentModesKHR(m_surface);
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
                                           m_surface,
                                           caps.minImageCount +1,
                                           selected_format.format,
                                           selected_format.colorSpace,
                                           extent,
                                           1,
                                           vk::ImageUsageFlagBits::eColorAttachment
                                           );
    if (graphics_family_index != present_family_index) {
        create_info.setQueueFamilyIndices(queue_family_indices);
    } else {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
    }
    create_info.preTransform = caps.currentTransform;
    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = selected_present_mode;
    create_info.clipped = VK_TRUE;

    m_swapchain = vk::raii::SwapchainKHR(*m_device, create_info);
    create_image_views(selected_format);
};

void swapchain_manager::cleanup_swapchain() {
    m_device->waitIdle();
    m_image_views.clear();
    m_swapchain = nullptr;
}

void swapchain_manager::create_image_views(vk::SurfaceFormatKHR selected_format) {
    std::vector<vk::Image> swapchain_images = m_swapchain.getImages();
    m_image_views.reserve(swapchain_images.size());
    for (auto& image : swapchain_images) {
        vk::ImageViewCreateInfo create_info(
                                            {},
                                            image,
                                            vk::ImageViewType::e2D,
                                            selected_format.format,
                                            { 
                                                vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity 
                                            },
                                            {
                                                vk::ImageAspectFlagBits::eColor, // 这是一个颜色附件
                                                0, // 起始 Mip 层级
                                                1, // 层级数量
                                                0, // 起始阵列层
                                                1  // 阵列层数量
                                            }
                                            );
        m_image_views.emplace_back(*m_device, create_info);
    }
}

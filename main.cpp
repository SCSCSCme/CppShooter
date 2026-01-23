#if defined(__APPLE__) || defined(__MACH__)
#error "Error: This program doesn't support macOS."
#endif

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "managers.h"

struct queue_family_indices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};



struct properties {
    int32_t window_width = 800;
    int32_t window_height = 600;
    const char* app_name	= "CppShooter";

    std::vector<const char *> required_extensions = {
#ifndef NDEBUG
        "VK_EXT_debug_utils",
#endif
    };

    std::vector<const char*> required_layers = {
#ifndef NDEBUG					// DEBUG mode enable validation layer
        "VK_LAYER_KHRONOS_validation",
#endif
    };

    std::vector<const char*> device_required_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
	
	std::set<uint32_t> unique_families{};
};

struct vulkan_context {
    vk::raii::Context context;
    GLFWwindow* window = nullptr;
    vk::raii::Instance instance = nullptr;
#ifndef NDEBUG
    vk::raii::DebugUtilsMessengerEXT debug_messenger = nullptr;
#endif
    vk::raii::SurfaceKHR surface = nullptr; 
    vk::raii::PhysicalDevice phys_device = nullptr;
    vk::raii::Device device = nullptr;
    std::optional<swapchain_manager> swapchain_mgr;
	queue_family_indices selected_queue_families;
};

/*************************
 * The functions         *
 * ***********************/
void setup_envrioment(properties& props) {
    spdlog::info("Setup envrioment. ");
    if (!glfwInit()) {
        spdlog::error("Failed to init glfw. ");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const char* platform = "unknown";
    switch (glfwGetPlatform()) {
    case GLFW_PLATFORM_WAYLAND: platform = "Wayland"; break;
    case GLFW_PLATFORM_X11: platform = "X11"; break;
    case GLFW_PLATFORM_COCOA: platform = "macOS"; break;
    default: break;
    }
    spdlog::info("GLFW using platform: {}", platform);

    spdlog::info("Create context");
    uint32_t apiVersion = vk::enumerateInstanceVersion();
    spdlog::info("Vulkan api version is: {:x}", apiVersion);
    if (apiVersion < 0x30000) {
        spdlog::error("Vulkan api version is too low. ");
    }
    spdlog::info("Vulkan headers version: {}", VK_HEADER_VERSION);

    // Get glfw required extensions
    uint32_t glfwRequiredExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
    for (uint32_t i = 0; i < glfwRequiredExtensionCount; i++) {
        props.required_extensions.push_back(glfwExtensions[i]);
    }
}

void create_window(vulkan_context& ctx, const properties& props) {
    spdlog::info("Create window. ");
    if (props.window_height < 600 || props.window_width < 800) {
        spdlog::error("Window size is too small. ");
    } 
    ctx.window = glfwCreateWindow(props.window_width, props.window_height, "CppShooter", nullptr, nullptr); 
    if (ctx.window == nullptr) {
        spdlog::error("Failed to create window");
    }
    glfwShowWindow(ctx.window);
}

bool check_extensions_layers(vulkan_context& ctx, const properties& props) {
    bool all_available = true;

    // Check the other extensions
    if (!props.required_extensions.empty()) {
        auto available_extensions = ctx.context.enumerateInstanceExtensionProperties();
        std::set<std::string> available_extensions_set;
        for (const auto& ext : available_extensions) {
            available_extensions_set.insert(ext.extensionName);
        }

        for (const char* ext : props.required_extensions) {
            if (available_extensions_set.find(ext) == available_extensions_set.end()) {
                spdlog::info("Missing required extension: {}", ext);
                all_available = false;
            }
        }
    }

    // Check the layers
    if (!props.required_layers.empty()) {
        auto available_layers = ctx.context.enumerateInstanceLayerProperties();
        std::set<std::string> available_layer_set;

        for (const auto& layer : available_layers) {
            available_layer_set.insert(layer.layerName);
        }

        for (const char* layer : props.required_layers) {
            if (available_layer_set.find(layer) == available_layer_set.end()) {
                printf("Missing required layer: %s\n", layer);
                all_available = false;

            }
        }
    }	

    return all_available;
}

void create_instance(vulkan_context& ctx, const properties& props) {
    spdlog::info("Create instance. ");
    vk::ApplicationInfo appInfo(
                                props.app_name,            
                                VK_MAKE_VERSION(1, 0, 0),                 
                                "No Engine",                             
                                VK_MAKE_VERSION(1, 0, 0),             
                                VK_API_VERSION_1_3
                                );

    if (!check_extensions_layers(ctx, props)) {
        spdlog::error("Required extensions and layers is unavailable. ");
    }

    vk::InstanceCreateInfo createInfo(
                                      vk::InstanceCreateFlags(),
                                      &appInfo,
                                      static_cast<uint32_t>(props.required_layers.size()),
                                      props.required_layers.data(),
                                      static_cast<uint32_t>(props.required_extensions.size()),
                                      props.required_extensions.data()
                                      );
    try {
        ctx.instance = vk::raii::Instance(ctx.context, createInfo);
    } catch (const std::exception& e) {
        spdlog::error("Failed to create Vulkan instance: {}", e.what());
        throw;
    }

    // 加上这一行防御性检查，如果这里过了，后面创建 Surface 就绝对不会因为 Instance 崩
    if (!(*ctx.instance)) {
        spdlog::error("Instance handle is still null after creation!");
        throw;
    }
    spdlog::info("Instance Dispatcher Pointer: {}", (void*)ctx.instance.getDispatcher());

    spdlog::info("--- TEST 1: Instance Integrity ---");
    try {
        // 尝试获取一个扩展函数的地址，但不执行它
        PFN_vkCreateDebugUtilsMessengerEXT pfn = 
            (PFN_vkCreateDebugUtilsMessengerEXT)ctx.instance.getDispatcher()->vkGetInstanceProcAddr(
                                                                                                    *ctx.instance, "vkCreateDebugUtilsMessengerEXT");
    
        if (pfn) {
            spdlog::info("Test 1 Passed: Found extension function address at {}", (void*)pfn);
        } else {
            spdlog::error("Test 1 Failed: vkGetInstanceProcAddr returned nullptr!");
        }
    } catch (...) {
        spdlog::error("Test 1 Failed: Crash during address lookup!");
    }
}

#ifndef NDEBUG
#include <iostream>
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
                                                     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData) {
    std::string type = "";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) type = "[Validation] ";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) type = "[Performance] ";
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
       spdlog::error("{}{}", type, pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
       spdlog::warn("{}{}", type, pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
       spdlog::info("{}{}", type, pCallbackData->pMessage);
    }
    else {
       spdlog::debug("{}{}", type, pCallbackData->pMessage);
    }
    return VK_FALSE;
}

void create_debug_messenger(vulkan_context& ctx) {
    spdlog::info("Create debug messgenger.");
    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
                                                    {},
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                                                    debug_callback,
                                                    nullptr
                                                    );
    spdlog::info("--- TEST 3: Debug Messenger Manual Creation ---");
    try {
        // 这种方式会强制使用 instance 的 dispatcher
        ctx.debug_messenger = ctx.instance.createDebugUtilsMessengerEXT(createInfo);
        spdlog::info("Test 3 Passed: Manual creation success!");
    } catch (const std::exception& e) {
        spdlog::error("Test 3 Failed: Exception - {}", e.what());
    }
}
#endif

void create_surface(vulkan_context& ctx) {
    spdlog::info("Create surface. ");
    VkSurfaceKHR glfw_surface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(
                                              *ctx.instance,
                                              ctx.window,
                                              nullptr,
                                              &glfw_surface
                                              );
    if (result != VK_SUCCESS) {
        spdlog::error("Failed to create surface. (GLFW)");
    }

    ctx.surface = vk::raii::SurfaceKHR(ctx.instance, vk::SurfaceKHR(glfw_surface));
    spdlog::info("Surface created successfully.");

    // 在 create_surface 执行完后立即插入
    spdlog::info("--- TEST 2: Surface Dispatcher Check ---");
    // 打印 Surface 的内部句柄
    spdlog::info("Surface Handle: {}", (void*)(VkSurfaceKHR)(*ctx.surface));

    // 这里的核心点：尝试通过 ctx.surface 间接访问 dispatcher
    if (ctx.surface.getDispatcher() == nullptr) {
        spdlog::error("Test 2 Failed: Surface has NO Dispatcher!");
    } else {
        spdlog::info("Test 2 Passed: Surface Dispatcher is at {}", (void*)ctx.surface.getDispatcher());
    }
}

bool check_device_extensions(const vk::raii::PhysicalDevice& device, const properties& props) {
    std::vector<vk::ExtensionProperties> available_extensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> required(props.device_required_extensions.begin(), props.device_required_extensions.end());
    for (const auto& extension : available_extensions) {
        required.erase(extension.extensionName);
    }

    return required.empty();
}

queue_family_indices find_queue_families(vk::raii::PhysicalDevice& device, vulkan_context& ctx) {
    queue_family_indices indices;
    auto queue_families = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queue_families.size(); ++i) {
        if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }
        if (device.getSurfaceSupportKHR(i, *ctx.surface)) {
            indices.present_family = i;
        }
        if (indices.is_complete()) break;
    }
    return indices;
}

void pick_physical_device(vulkan_context& ctx, const properties props) {
    spdlog::info("Pick physical device. ");
    std::vector<vk::raii::PhysicalDevice> physical_devices = ctx.instance.enumeratePhysicalDevices();
    if (physical_devices.size() == 0) {
        spdlog::error("Failed to find GPUs support Vulkan! ");
    }
    for (auto& device : physical_devices) {
        queue_family_indices indices = find_queue_families(device, ctx);
        if (!indices.is_complete()) continue;

        if (!check_device_extensions(device, props)) {
            spdlog::info("Device : {} isn't supported required extensions, Skip.", (const char*)device.getProperties().deviceName);
            continue;
        }
                
        // Check the required 
        auto formats = device.getSurfaceFormatsKHR(*ctx.surface);
        auto presentModes = device.getSurfacePresentModesKHR(*ctx.surface);
        if (formats.empty() || presentModes.empty()) continue;

        auto chain = device.getFeatures2<vk::PhysicalDeviceFeatures2,
                                         vk::PhysicalDeviceVulkan11Features,
                                         vk::PhysicalDeviceVulkan12Features>();

        // 2. 使用 .get<T>() 获取具体的引用
        const auto& features2 = chain.get<vk::PhysicalDeviceFeatures2>();
        const auto& features12 = chain.get<vk::PhysicalDeviceVulkan12Features>();

        if (!features2.features.samplerAnisotropy) continue;
        if (!features2.features.fillModeNonSolid) continue;
        if (!features2.features.geometryShader || !features12.bufferDeviceAddress) {
            spdlog::warn("Device doesn't support required features.");
            continue;
        }

		 ctx.selected_queue_families = indices;
        ctx.phys_device = device;
        if (ctx.phys_device.getDispatcher() == nullptr) {
            spdlog::error("Physical device doesn't alive. ");
        }
        return;
    }
    spdlog::error("Failed to pick physical device. ");
}

void create_device(vulkan_context& ctx, properties& props) {
    queue_family_indices indices = ctx.selected_queue_families;
    if (!indices.is_complete()) {
        spdlog::error("Failed to find queue families. ");
		throw std::runtime_error("Missing required queue families!");
    }

    props.unique_families = {
        indices.graphics_family.value(),
        indices.present_family.value()
    };

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    float queue_priority = 1.0f;

    for (uint32_t family : props.unique_families) {
        queue_create_infos.emplace_back(
                                        vk::DeviceQueueCreateFlags{},
                                        family,
                                        1,
                                        &queue_priority
                                        );
    }

    vk::PhysicalDeviceFeatures device_features{};  // 按需设 true
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::DeviceCreateInfo device_info{};
    device_info.setPQueueCreateInfos(queue_create_infos.data())
        .setPEnabledFeatures(&device_features)
        .setPEnabledExtensionNames(device_extensions);

    ctx.device = vk::raii::Device(ctx.phys_device, device_info);
}

void setup_swapchain(vulkan_context& ctx) {
    ctx.swapchain_mgr.emplace(
                              &ctx.device,   // 传入 raii::Device
                              &ctx.phys_device,
                              &ctx.surface,  // 传入 raii::Surface
                              ctx.window
                              );
}

void mainloop(vulkan_context& ctx) {
    spdlog::info("Mainloop start. ");

    if (!glfwGetWindowAttrib(ctx.window, GLFW_VISIBLE)) {
        spdlog::warn("Window not visible, showing...");
        glfwShowWindow(ctx.window);
    }

    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
    }
}

int main() {
    spdlog::info("Program run. ");
    vulkan_context context;
    properties property;
    setup_envrioment(property);
    create_window(context, property);
    create_instance(context, property);
    create_surface(context);
#ifndef NDEBUG
    create_debug_messenger(context);
#endif
	pick_physical_device(context, property);
    create_device(context, property);
    setup_swapchain(context);
    mainloop(context);
    return 0;
}

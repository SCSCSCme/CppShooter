#if defined(__APPLE__) || defined(__MACH__)
#error "Error: This program doesn't support macOS."
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

static int32_t window_width;
static int32_t window_height;
static const char* app_name	= "CppShooter";
static std::unique_ptr<vk::raii::Context> g_context	   = nullptr; // global context pointer
static std::unique_ptr<vk::raii::Instance> g_instance  = nullptr;
#ifndef NDEBUG
static vk::raii::DebugUtilsMessengerEXT g_debug_messenger = nullptr;
#endif
static std::unique_ptr<vk::raii::SurfaceKHR> g_surface = nullptr; 
static GLFWwindow* window = nullptr;
static std::unique_ptr<vk::raii::Device> g_device = nullptr;

struct queue_family_indices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value();
	}
};

static std::vector<const char *> required_extensions = {
#ifndef NDEBUG
	"VK_EXT_debug_utils",
#endif
};

static std::vector<const char*> required_layers = {
#ifndef NDEBUG					// DEBUG mode enable validation layer
	"VK_LAYER_KHRONOS_validation",
#endif
};

static std::vector<const char*> device_required_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void setup_envrioment() {
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
		case GLFW_PLATFORM_WIN32: platform = "Win32"; break;
	}
	spdlog::info("GLFW using platform: {}", platform);

	spdlog::info("Create context");
	uint32_t apiVersion = vk::enumerateInstanceVersion();
	spdlog::info("Vulkan api version is: {:x}", apiVersion);
	if (apiVersion < 0x40000) {
		spdlog::error("Vulkan api version is too low. ");
	}
	g_context = std::make_unique<vk::raii::Context>();
	spdlog::info("Vulkan headers version: {}", VK_HEADER_VERSION);
}

void create_window() {
	spdlog::info("Create window. ");
	if (window_height < 100 || window_width < 150) {
		spdlog::error("Window size is too small. ");
	} 
	window = glfwCreateWindow(window_width, window_height, "CppShooter", nullptr, nullptr); 
	if (window == nullptr) {
		spdlog::error("Failed to create window");
	}
	glfwShowWindow(window);
}

bool check_extensions_layers() {
	bool all_available = true;

	// Get glfw required extensions
	uint32_t glfwRequiredExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);

	for (uint32_t i = 0; i < glfwRequiredExtensionCount; i++) {
		required_extensions.push_back(glfwExtensions[i]);
	}	

	// Check the other extensions
	if (!required_extensions.empty()) {
		auto available_extensions = g_context->enumerateInstanceExtensionProperties();
		std::set<std::string> available_extensions_set;
		for (const auto& ext : available_extensions) {
			available_extensions_set.insert(ext.extensionName);
		}

		for (const char* ext : required_extensions) {
			if (available_extensions_set.find(ext) == available_extensions_set.end()) {
				printf("Missing required extension: %s\n", ext);
				all_available = false;
			}
		}
	}

	// Check the layers
	if (!required_layers.empty()) {
		auto available_layers = g_context->enumerateInstanceLayerProperties();
		std::set<std::string> available_layer_set;

		for (const auto& layer : available_layers) {
			available_layer_set.insert(layer.layerName);
		}

		for (const char* layer : required_layers) {
			if (available_layer_set.find(layer) == available_layer_set.end()) {
				printf("Missing required layer: %s\n", layer);
				all_available = false;
			}
		}
	}	

	return all_available;
}

void create_instance() {
	spdlog::info("Create instance. ");
	vk::ApplicationInfo appInfo(
			app_name,                          		// pApplicationName
			VK_MAKE_VERSION(1, 0, 0),                  // applicationVersion
			"No Engine",                               // pEngineName
			VK_MAKE_VERSION(1, 0, 0),                  // engineVersion
			vk::ApiVersion14                           // apiVersion (use a recent version)
			);

	if (!check_extensions_layers()) {
		spdlog::error("Required extensions and layers is unavailable. ");
	}

	vk::InstanceCreateInfo createInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			static_cast<uint32_t>(required_layers.size()),
			required_layers.data(),
			static_cast<uint32_t>(required_extensions.size()),
			required_extensions.data()
			);
	g_instance = std::make_unique<vk::raii::Instance>(*g_context, createInfo);
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

void create_debug_messenger() {
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

	g_debug_messenger = vk::raii::DebugUtilsMessengerEXT(*g_instance, createInfo);
}
#endif

void create_surface() {
	spdlog::info("Create surface. ");
	VkSurfaceKHR glfw_surface = VK_NULL_HANDLE;
	VkResult result = glfwCreateWindowSurface(
			**g_instance,
			window,
			nullptr,
			&glfw_surface
			);
	if (result != VK_SUCCESS) {
		spdlog::error("Failed to create surface. (GLFW)");
	}

	g_surface = std::make_unique<vk::raii::SurfaceKHR>(*g_instance, vk::SurfaceKHR(glfw_surface));
}

bool check_device_extensions(const vk::raii::PhysicalDevice& device) {
	std::vector<vk::ExtensionProperties> available_extensions = device.enumerateDeviceExtensionProperties();
	std::set<std::string> required(device_required_extensions.begin(), device_required_extensions.end());
	for (const auto& extension : available_extensions) {
		required.erase(extension.extensionName);
	}

	return required.empty();
}

queue_family_indices find_queue_families(const vk::raii::PhysicalDevice& device) {
	queue_family_indices indices;
	auto queue_families = device.getQueueFamilyProperties();

	for (uint32_t i = 0; i < queue_families.size(); ++i) {
		if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphics_family = i;
		}
		if (device.getSurfaceSupportKHR(i, **g_surface)) {
			indices.present_family = i;
		}
		if (indices.is_complete()) break;
	}
	return indices;
}

vk::raii::PhysicalDevice pick_physical_device() {
	vk::raii::PhysicalDevices physical_devices(*g_instance);
	if (physical_devices.size() == 0) {
		spdlog::error("Failed to find GPUs with Vulkan Support! ");
	}
	if (g_surface.get() == nullptr) {
		spdlog::error("Surface is null! ");
	}
	for (const auto& device : physical_devices) {
		queue_family_indices indices = find_queue_families(device);
		if (!indices.is_complete()) continue;

		if (!check_device_extensions(vk::raii::PhysicalDevice(device))) {
			spdlog::info("Device : {} isn't supported required extensions, Skip.", (const char*)device.getProperties().deviceName);
			continue;
		}

		auto formats = device.getSurfaceFormatsKHR(**g_surface);
		auto presentModes = device.getSurfacePresentModesKHR(**g_surface);
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

		return vk::raii::PhysicalDevice(device);
	}
	spdlog::error("Failed to pick physical device. ");
}

void create_device() {
	vk::raii::PhysicalDevice physical_device(pick_physical_device());
	queue_family_indices indices = find_queue_families(physical_device);

	std::set<uint32_t> unique_families = {
		indices.graphics_family.value(),
		indices.present_family.value()
	};

	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
	float queue_priority = 1.0f;

	for (uint32_t family : unique_families) {
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

	g_device = std::make_unique<vk::raii::Device>(physical_device, device_info);
}

void mainloop() {
	spdlog::info("Mainloop start. ");

	if (!glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
		spdlog::warn("Window not visible, showing...");
		glfwShowWindow(window);
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void clean_resource() {
	spdlog::info("Clean resource. ");
	g_surface.reset();
	g_instance.reset();
	g_device.reset();
	glfwTerminate();
}

int main() {
	spdlog::info("Program run. ");
	setup_envrioment();

	window_width = 1280;
	window_height = 720;
	create_window();
	create_instance();
	create_surface();
#ifndef NDEBUG
	create_debug_messenger();
#endif
	create_device();
	mainloop();
	clean_resource();
	return 0;
}

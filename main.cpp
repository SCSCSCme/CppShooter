#include <spdlog/spdlog.h>
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <set>

static int32_t window_width;
static int32_t window_height;
static const char* app_name	= "CppShooter";
static std::unique_ptr<vk::raii::Context> g_context	  = nullptr; // global context pointer
static std::unique_ptr<vk::raii::Instance> g_instance = nullptr;
static GLFWwindow* window = nullptr;
static std::vector<const char*> required_extensions = {
	"VK_KHR_wayland_surface",
	"VK_KHR_surface",
#ifndef NDEBUG
	"VK_EXT_debug_utils",
#endif
};

static std::vector<const char*> required_layers = {
#ifndef NDEBUG					// DEBUG mode enable validation layer
	"VK_LAYER_KHRONOS_validation",
#endif
};

void setup_envrioment() {
	spdlog::info("Setup envrioment. ");
	if (!glfwInit()) {
		spdlog::error("Failed to init glfw. ");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
}

bool check_extensions_layers() {
	bool all_available = true;
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

void mainloop() {
  while (!glfwWindowShouldClose(window)) {
  }
}
void clean_resource() {
	spdlog::info("Clean resource. ");
	glfwTerminate();
}

int main() {
  spdlog::info("Program run. ");
  setup_envrioment();

  window_width = 1280;
  window_height = 720;
  create_window();
  create_instance();
  mainloop();

  clean_resource();
  return 0;
}

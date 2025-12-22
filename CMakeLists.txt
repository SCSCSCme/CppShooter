set(PROJECT_NAME CppShooter)
cmake_minimum_required(VERSION 3.12)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

message(STATUS "Vulkan include dir: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan library: ${Vulkan_LIBRARIES}")

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE 
	spdlog 
	fmt::fmt 
	Vulkan::Vulkan
	glfw
)

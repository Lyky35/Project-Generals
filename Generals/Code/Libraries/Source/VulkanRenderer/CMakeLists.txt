add_library(VulkanRenderer STATIC)

target_include_directories(VulkanRenderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_compile_definitions(VulkanRenderer PUBLIC HAS_VULKAN=1)
    target_link_libraries(VulkanRenderer PUBLIC Vulkan::Vulkan)
else()
    target_compile_definitions(VulkanRenderer PUBLIC HAS_VULKAN=0)
endif()

file(GLOB VULKAN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
target_sources(VulkanRenderer PRIVATE ${VULKAN_SOURCES})

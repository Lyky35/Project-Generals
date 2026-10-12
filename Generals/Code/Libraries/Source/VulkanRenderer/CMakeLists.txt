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

find_program(GLSLANG_VALIDATOR glslangValidator)
if(GLSLANG_VALIDATOR)
    file(GLOB VULKAN_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag)
    foreach(SHADER ${VULKAN_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        add_custom_command(
            OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME}.spv
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
            COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3 ${SHADER} -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME}.spv
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
        )
        list(APPEND VULKAN_SPV ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME}.spv)
    endforeach()
    add_custom_target(VulkanShaders ALL DEPENDS ${VULKAN_SPV})
    add_dependencies(VulkanRenderer VulkanShaders)
endif()

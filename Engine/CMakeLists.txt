set(ENGINE_ROOT_DIR "${MINIENGINE_ROOT_DIR}/Engine")
set(THIRD_PARTY_DIR "${ENGINE_ROOT_DIR}/ThirdParty")

set(ENGINE_ASSET_DIR "/Assets")

set(DEPLOY_CONFIG_DIR "Configs/Deployment")
set(DEVELOP_CONFIG_DIR "Configs/Development")

# 设置Vulkan
set(vulkan_include ${THIRD_PARTY_DIR}/VulkanSDK/include)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")   # enable parallel compilation
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MiniEngineEditor)
endif()

if(WIN32) # 对于不同平台拥有不同设置
    set(vulkan_lib ${THIRD_PARTY_DIR}/VulkanSDK/lib/Win32/vulkan-1.lib)
    # 使用glslangValidator编译shader
    set(glslangValidator_executable ${THIRD_PARTY_DIR}/VulkanSDK/bin/Win32/glslangValidator.exe)
    add_compile_definitions("MINIENGINE_VK_LAYER_PATH=${THIRD_PARTY_DIR}/VulkanSDK/bin/Win32")
    message(NOTICE "Use Win32 Vulkan")
elseif(UNIX)
    if(APPLE)
        set(vulkan_lib ${THIRD_PARTY_DIR}/VulkanSDK/lib/MacOS/libvulkan.1.dylib)
        set(glslangValidator_executable ${THIRD_PARTY_DIR}/VulkanSDK/bin/MacOS/glslangValidator)
        add_compile_definitions("MINIENGINE_VK_LAYER_PATH=${THIRD_PARTY_DIR}/VulkanSDK/bin/MacOS")
        add_compile_definitions("MINIENGINE_VK_ICD_FILENAMES=${THIRD_PARTY_DIR}/VulkanSDK/bin/MacOS/MoltenVK_icd.json")
    else()
        set(vulkan_lib ${THIRD_PARTY_DIR}/VulkanSDK/lib/Linux/libvulkan.so.1)
        set(glslangValidator_executable ${THIRD_PARTY_DIR}/VulkanSDK/bin/Linux/glslangValidator)
        add_compile_definitions("MINIENGINE_VK_LAYER_PATH=${THIRD_PARTY_DIR}/VulkanSDK/bin/Linux")
    endif()
else()
    message(FATAL_ERROR "Unknown Platform")
endif()

set(SHADER_COMPILE_TARGET MiniEngineShaderCompile)
add_subdirectory(Shader)

add_subdirectory(ThirdParty)
add_subdirectory(MEditor)
add_subdirectory(MRuntime)
add_subdirectory(Parser)

set(CODEGEN_TARGET "PreCompile")
include(Parser/Precompile/precompile.cmake)
set_target_properties("${CODEGEN_TARGET}" PROPERTIES FOLDER "Engine" )
add_dependencies(MiniEngineRuntime "${CODEGEN_TARGET}")
add_dependencies("${CODEGEN_TARGET}" "Parser")
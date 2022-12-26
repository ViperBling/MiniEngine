set(TARGET_NAME MiniEngineRuntime)

file(GLOB_RECURSE RUNTIME_HEADERS CONFIGURE_DEPENDS "*.h")
file(GLOB_RECURSE RUNTIME_SOURCES CONFIGURE_DEPENDS "*.cpp")

# 作为库被构建
add_library(${TARGET_NAME} ${RUNTIME_HEADERS} ${RUNTIME_SOURCES})

# 链接库
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC ${vulkan_lib})

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")
message(NOTICE "Force using MSVC")

target_include_directories(
        ${TARGET_NAME}
        PUBLIC $<BUILD_INTERFACE:${ENGINE_ROOT_DIR}/Source>
        $<INSTALL_INTERFACE:${TARGET_NAME}-${PROJECT_VERSION}>
)

target_include_directories(
        ${TARGET_NAME}
        PUBLIC $<BUILD_INTERFACE:${ENGINE_ROOT_DIR}/Source/MRuntime>
        $<INSTALL_INTERFACE:${TARGET_NAME}-${PROJECT_VERSION}>
)

# Vulkan
target_include_directories(
        ${TARGET_NAME}
        PUBLIC $<BUILD_INTERFACE:${vulkan_include}>)

target_include_directories(
        ${TARGET_NAME}
        PUBLIC $<BUILD_INTERFACE:${THIRD_PARTY_DIR}/vulkanmemoryallocator/include>
)
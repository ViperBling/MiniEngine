set(THIRD_PARTY_FOLDER "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TEST "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${THIRD_PARTY_FOLDER}/glfw)
    set_target_properties(update_mappings PROPERTIES FOLDER ${THIRD_PARTY_FOLDER}/glfw) # gamepad mapping
endif()

if(NOT TARGET imgui)
    file(GLOB imgui_header 
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.h
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.h)
    file(GLOB imgui_source 
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp)
    add_library(imgui ${imgui_header} ${imgui_source})
    target_link_libraries(imgui PUBLIC glfw)
    target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui LINKER_LANGUAGE CXX)
endif()

if(NOT TARGET stb)
    file(GLOB stb_sources CONFIGURE_DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/stb/*.h)
    add_library(stb INTERFACE ${stb_sources})
    target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)
    set_target_properties(stb PROPERTIES FOLDER ${third_party_folder}/stb)
endif()

if(NOT TARGET json11)
    add_subdirectory(json11)
    set_target_properties(json11 PROPERTIES FOLDER ${THIRD_PARTY_FOLDER}/json11)
endif()

if(NOT TARGET spdlog)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder}/spdlog)
endif()
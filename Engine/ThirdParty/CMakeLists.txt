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

if(NOT TARGET json11)
    add_subdirectory(json11)
    set_target_properties(json11 PROPERTIES FOLDER ${THIRD_PARTY_FOLDER}/json11)
endif()
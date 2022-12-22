set(ENGINE_ROOT_DIR "${MINIENGINE_ROOT_DIR}/Engine")
set(THIRD_PARTY_DIR "${ENGINE_ROOT_DIR}/ThirdParty")

add_subdirectory(ThirdParty)

add_subdirectory(Source/MEditor)
add_subdirectory(Source/MRuntime)
project (imgui_collections)

file(GLOB_RECURSE IMGUI_COLLECTIONS_SOURCES "*.cpp")

add_library(imgui_collections STATIC ${IMGUI_COLLECTIONS_SOURCES})
target_include_directories(imgui_collections PUBLIC imgui)
target_include_directories(imgui_collections PUBLIC implot)
target_include_directories(imgui_collections PUBLIC cimgui)
target_include_directories(imgui_collections PUBLIC cimplot)
target_include_directories(imgui_collections PUBLIC ../freetype/include)

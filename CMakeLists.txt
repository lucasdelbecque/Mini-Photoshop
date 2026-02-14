cmake_minimum_required(VERSION 3.10)
project(MiniPhotoshop)

set(CMAKE_CXX_STANDARD 17)

# --- CONFIGURATION MAC ---
include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# --- INCLUDES ---
# On ajoute le dossier imgui aux includes pour pouvoir faire #include "imgui.h"
include_directories(dependencies/imgui)
include_directories(dependencies/include)

# --- FICHIERS SOURCES ---
# On liste tous les fichiers de ImGui
file(GLOB IMGUI_SOURCES "dependencies/imgui/*.cpp")

# --- LIBRAIRIES ---
find_package(OpenGL REQUIRED)
find_library(GLFW_LIB glfw)

# --- EXECUTABLE ---
# On ajoute IMGUI_SOURCES à la compilation
add_executable(MiniPhotoshop main.cpp ${IMGUI_SOURCES})

# --- LINK ---
if (APPLE)
    target_link_libraries(MiniPhotoshop ${GLFW_LIB} "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
else()
    target_link_libraries(MiniPhotoshop glfw OpenGL::GL)
endif()
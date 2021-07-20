cmake_minimum_required(VERSION 3.4)

project(test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Wextra -O3")

add_subdirectory(third_party)

# --- OpenGL ---
find_package(OpenGL REQUIRED)
# --------------

# --- GLFW ---
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(third_party/glfw)
add_definitions(-DGLFW_INCLUDE_NONE)
# ------------

# --- GLAD ---
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PRIVATE third_party/glad/include)
# ------------

# --- IMGUI ---
file(GLOB IMGUI_SOURCES
    third_party/imgui/*.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PRIVATE
    third_party/imgui/
    third_party/imgui/backends
    third_party/glfw/include
    third_party/glad/include
)
target_link_libraries(imgui PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    glad
)
set_target_properties(imgui PROPERTIES LINKER_LANGUAGE CXX)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
# ------------

add_executable(main
    src/main.cc
    src/graphics.cc
    src/vra.cc
)

target_include_directories(main PRIVATE
    SYSTEM third_party
    SYSTEM third_party/glad/include
    SYSTEM third_party/glfw/include
    SYSTEM third_party/glm
    SYSTEM third_party/imgui
    src
)

target_link_libraries(main
    ${OPENGL_LIBRARIES}
    glad
    glfw
    imgui
    libdeflate
)

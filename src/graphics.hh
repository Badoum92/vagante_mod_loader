#pragma once

#include <string>

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_glfw.h>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

GLFWwindow* create_window();
void destroy_window(GLFWwindow* window);
void update_window(GLFWwindow* window);
bool should_close(GLFWwindow* window);
void init_imgui(GLFWwindow* window);
void render();
GLuint load_texture(const std::string& path);
void destroy_texture(GLuint texture);

#pragma once

#include <string>

#include <GLFW/glfw3.h>

#include "types.hh"

constexpr float WINDOW_SCALE = 3.0f;
constexpr int WINDOW_WIDTH = 288 * WINDOW_SCALE;
constexpr int WINDOW_HEIGHT = 272 * WINDOW_SCALE;

extern GLFWwindow* g_window;

GLFWwindow* create_window();
void destroy_window(GLFWwindow* window);
void update_window(GLFWwindow* window);
bool should_close(GLFWwindow* window);
void init_imgui(GLFWwindow* window);

Texture load_texture(const std::string& path);
void destroy_texture(Texture& texture);

void render(const RenderData& render_data, VraData& vra_data, bool buttons = true);

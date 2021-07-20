#include "graphics.hh"

#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

GLFWwindow* create_window()
{
    if (!glfwInit())
    {
        throw std::runtime_error("Could not initialize glfw");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vagante Mod Loader", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Could not initialize glad");
    }

    init_imgui(window);

    glfwSwapInterval(1);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    return window;
}

void destroy_window(GLFWwindow* window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void update_window(GLFWwindow* window)
{
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool should_close(GLFWwindow* window)
{
    return glfwWindowShouldClose(window);
}

void init_imgui(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");
}

Texture load_texture(const std::string& path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (data == nullptr)
    {
        throw std::runtime_error("Could not load image " + path);
    }

    Texture texture = {};
    texture.size.x = width;
    texture.size.y = height;

    glCreateTextures(GL_TEXTURE_2D, 1, &texture.handle);
    glTextureStorage2D(texture.handle, 1, GL_RGBA8, width, height);
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTextureSubImage2D(texture.handle, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return texture;
}

void destroy_texture(Texture& texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
    texture.size = {0, 0};
}

void render(const RenderData& render_data)
{
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Test");
    ImGui::Image((void*)&render_data.texture.handle, render_data.texture.size);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();
}

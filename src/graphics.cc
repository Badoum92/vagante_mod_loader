#include "graphics.hh"

#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_glfw.h>
#include <imfilebrowser.h>

#include "vra.hh"

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

    // glfwSwapInterval(1);
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

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("assets/vagante.ttf", 20.0f);
}

Texture load_texture(const std::string& path)
{
    int width = 0;
    int height = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, nullptr, 4);

    if (data == nullptr)
    {
        throw std::runtime_error("Could not load image " + path);
    }

    Texture texture = {};
    texture.size.x = width;
    texture.size.y = height;

    glCreateTextures(GL_TEXTURE_2D, 1, &texture.handle);
    glTextureParameteri(texture.handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texture.handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureStorage2D(texture.handle, 1, GL_RGBA8, width, height);
    glTextureSubImage2D(texture.handle, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    texture.imgui_ptr = (void*)(intptr_t)texture.handle;
    return texture;
}

void destroy_texture(Texture& texture)
{
    glDeleteTextures(1, &texture.handle);
    texture.handle = 0;
    texture.size = {0, 0};
}

static ImVec2 operator*(const ImVec2& vec2, float f)
{
    return ImVec2(vec2.x * f, vec2.y * f);
}

static void imgui_set_cursor_pos(int x, int y)
{
    ImGui::SetCursorPos(ImVec2(x, y) * WINDOW_SCALE);
}

static void imgui_image(const Texture& texture)
{
    ImGui::Image(texture.imgui_ptr, texture.size * WINDOW_SCALE);
}

static bool imgui_image_button(const Texture& texture, bool button, int id)
{
    if (button)
    {
        ImGui::PushID(id);
        bool ret = ImGui::ImageButton(texture.imgui_ptr, texture.size * WINDOW_SCALE);
        ImGui::PopID();
        return ret;
    }
    else
    {
        imgui_image(texture);
        return false;
    }
}

static void imgui_progress_bar(int x, int y, const Texture& empty, const Texture& full, float progress)
{
    imgui_set_cursor_pos(x, y);
    ImGui::Image(empty.imgui_ptr, empty.size * WINDOW_SCALE);
    imgui_set_cursor_pos(x, y);
    ImGui::Image(full.imgui_ptr, ImVec2(full.size.x * WINDOW_SCALE * progress, full.size.y * WINDOW_SCALE),
                 ImVec2(0, 0), ImVec2(progress, 1));
}

static void imgui_mod_list(VraData& vra_data)
{
    ImGui::BeginListBox("", ImVec2(100, 85) * WINDOW_SCALE);
    for (size_t i = 0; i < vra_data.mods.size(); ++i)
    {
        bool selected = vra_data.mods[i].selected;
        if (ImGui::Selectable(vra_data.mods[i].name.c_str(), &selected))
        {
            auto selected_idx = vra_data.get_selected_mod();
            if (selected_idx.has_value())
            {
                vra_data.mods[selected_idx.value()].selected = false;
            }
            vra_data.mods[i].selected = true;
        }
    }
    ImGui::EndListBox();
}

static void imgui_log(const VraData& vra_data)
{
    static int log_size = vra_data.log_buffer.size();
    ImGui::SetNextWindowPos(ImVec2(8, 200) * WINDOW_SCALE);
    ImGui::BeginChild("Log", ImVec2(130, 68) * WINDOW_SCALE, false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(vra_data.log_buffer.c_str(), vra_data.log_buffer.c_str() + vra_data.log_buffer.size());
    if (log_size != vra_data.log_buffer.size())
    {
        log_size = vra_data.log_buffer.size();
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

static void imgui_text(int x, int y, const ImVec2& size, const std::string& text, const std::string& child_name)
{
    ImGui::SetNextWindowPos(ImVec2(x, y) * WINDOW_SCALE);
    ImGui::BeginChild(child_name.c_str(), size * WINDOW_SCALE, false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
    ImGui::EndChild();
}

static void end_frame()
{
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(2);
    ImGui::End();
    ImGui::EndFrame();
}

static void unpack(VraData& vra_data)
{
    if (vra_data.unpack_file.empty())
    {
        vra_data.log_buffer.appendf("Select a .vra file to unpack\n");
        end_frame();
        return;
    }

    try
    {
        std::filesystem::path src = vra_data.unpack_file;
        std::filesystem::path dst = src;
        dst.replace_extension("");

        vra_data.log_buffer.clear();
        vra_data.progress = 0.0f;
        end_frame();

        vra_data.log_buffer.appendf("Unpacking...\n");
        unpack_file(src, dst);
        vra_data.log_buffer.appendf("Done!\n");
        vra_data.log_buffer.appendf("%ls directory is in: %ls\n", dst.filename().c_str(), dst.parent_path().c_str());
    }
    catch (const std::exception& e)
    {
        vra_data.log_buffer.appendf("%s\n", e.what());
    }
}

static void repack(VraData& vra_data)
{
    if (vra_data.repack_file.empty())
    {
        vra_data.log_buffer.appendf("Select a directory to repack\n");
        end_frame();
        return;
    }

    try
    {
        std::filesystem::path src = vra_data.repack_file;
        std::filesystem::path dst = src;
        dst.replace_extension(".vra");

        vra_data.log_buffer.clear();
        vra_data.progress = 0.0f;
        end_frame();

        vra_data.log_buffer.appendf("Repacking...\n");
        pack_dir(src, dst);
        vra_data.log_buffer.appendf("Done!\n");
        vra_data.log_buffer.appendf("%ls is in: %ls\n", dst.filename().c_str(), dst.parent_path().c_str());
    }
    catch (const std::exception& e)
    {
        vra_data.log_buffer.appendf("%s\n", e.what());
    }
}

static void load_mods(VraData& vra_data)
{
    if (vra_data.data_vra.empty())
    {
        vra_data.log_buffer.appendf("Select a .vra file to mod\n");
        end_frame();
        return;
    }

    if (vra_data.mod_dir.empty())
    {
        vra_data.log_buffer.appendf("Select mod directory\n");
        end_frame();
        return;
    }

    try
    {
        std::filesystem::path src = vra_data.data_vra;
        std::filesystem::path dst = vra_data.data_vra;
        dst.replace_filename("modded_" + src.filename().string());
        vra_data.log_buffer.clear();
        vra_data.progress = 0.0f;
        end_frame();

        vra_data.log_buffer.appendf("Loading mods...\n");
        load_mods(vra_data.data_vra);
        vra_data.log_buffer.appendf("Done!\n");
        vra_data.log_buffer.appendf("%ls is in %ls\n", dst.filename().c_str(), dst.parent_path().c_str());
    }
    catch (const std::exception& e)
    {
        vra_data.log_buffer.appendf("%s\n", e.what());
    }
}

void render(const RenderData& render_data, VraData& vra_data, bool buttons)
{
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    if (!ImGui::Begin("Mod Loader", NULL,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                          | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar
                          | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse
                          | ImGuiWindowFlags_NoBackground))
    {
        ImGui::EndFrame();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1, 0.1, 0.1, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2, 0.2, 0.2, 1));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1, 0.1, 0.1, 1));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2, 0.2, 0.2, 1));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3, 0.3, 0.3, 1));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2, 0.2, 0.2, 1));

    imgui_image(render_data.background);

    static ImGui::FileBrowser browse_unpack(0);
    browse_unpack.SetTitle("Select data.vra to unpack");
    browse_unpack.SetTypeFilters({".vra"});
    imgui_set_cursor_pos(10, 77);
    if (imgui_image_button(render_data.unpack, buttons, 1))
    {
        unpack(vra_data);
        return;
    }
    imgui_set_cursor_pos(100, 79);
    if (imgui_image_button(render_data.browse, buttons, 2))
    {
        browse_unpack.Open();
    }
    imgui_text(14, 104, ImVec2(112, 12), vra_data.unpack_file, "unpack child");

    static ImGui::FileBrowser browse_repack(ImGuiFileBrowserFlags_SelectDirectory);
    browse_repack.SetTitle("Select directory to repack");
    imgui_set_cursor_pos(10, 116);
    if (imgui_image_button(render_data.repack, buttons, 3))
    {
        repack(vra_data);
        return;
    }
    imgui_set_cursor_pos(100, 118);
    if (imgui_image_button(render_data.browse, buttons, 4))
    {
        browse_repack.Open();
    }
    imgui_text(14, 143, ImVec2(112, 12), vra_data.repack_file, "repack child");

    static ImGui::FileBrowser browse_datavra(0);
    browse_datavra.SetTitle("Select data.vra to mod");
    browse_datavra.SetTypeFilters({".vra"});
    imgui_set_cursor_pos(212, 25);
    if (imgui_image_button(render_data.browse, buttons, 5))
    {
        browse_datavra.Open();
    }
    imgui_text(157, 48, ImVec2(112, 12), vra_data.data_vra, "data.vra");

    static ImGui::FileBrowser browse_mods(ImGuiFileBrowserFlags_SelectDirectory);
    browse_mods.SetTitle("Select your mods directory");
    imgui_set_cursor_pos(217, 60);
    if (imgui_image_button(render_data.browse, buttons, 6))
    {
        browse_mods.Open();
    }
    imgui_text(157, 85, ImVec2(112, 12), vra_data.mod_dir, "mod folder");

    imgui_progress_bar(10, 163, render_data.progress_empty, render_data.progress_full, vra_data.progress);

    imgui_set_cursor_pos(152, 100);
    imgui_mod_list(vra_data);

    imgui_set_cursor_pos(255, 107);
    if (imgui_image_button(render_data.up, buttons, 7))
    {
        vra_data.mod_up();
    }

    imgui_set_cursor_pos(255, 151);
    if (imgui_image_button(render_data.down, buttons, 8))
    {
        vra_data.mod_down();
    }

    imgui_set_cursor_pos(187, 199);
    if (imgui_image_button(render_data.load_mods, buttons, 9))
    {
        load_mods(vra_data);
        return;
    }

    imgui_log(vra_data);

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(2);

    ImGui::End();

    browse_unpack.Display();
    browse_repack.Display();
    browse_datavra.Display();
    browse_mods.Display();

    if (browse_unpack.HasSelected())
    {
        vra_data.unpack_file = browse_unpack.GetSelected().string();
        browse_unpack.ClearSelected();
    }
    if (browse_repack.HasSelected())
    {
        vra_data.repack_file = browse_repack.GetSelected().string();
        browse_repack.ClearSelected();
    }
    if (browse_datavra.HasSelected())
    {
        vra_data.data_vra = browse_datavra.GetSelected().string();
        browse_datavra.ClearSelected();
    }
    if (browse_mods.HasSelected())
    {
        vra_data.mod_dir = browse_mods.GetSelected().string();
        vra_data.mods.clear();
        for (auto& dirent : std::filesystem::directory_iterator(browse_mods.GetSelected()))
        {
            vra_data.mods.push_back({dirent.path().filename().string(), false});
        }
        browse_mods.ClearSelected();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();

    update_window(g_window);
}

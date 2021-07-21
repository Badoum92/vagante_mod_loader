#pragma once

#include <string>
#include <vector>
#include <optional>
#include <glad/glad.h>
#include <imgui.h>

struct Texture
{
    GLuint handle = 0;
    void* imgui_ptr = nullptr;
    ImVec2 size{0, 0};
};

struct RenderData
{
    Texture background;
    Texture browse;
    Texture unpack;
    Texture repack;
    Texture up;
    Texture down;
    Texture load_mods;
    Texture progress_empty;
    Texture progress_full;
};

struct Mod
{
    std::string name;
    bool selected;
};

struct VraData
{
    std::string unpack_file;
    std::string repack_file;

    std::string data_vra;
    std::string mod_dir;
    std::vector<Mod> mods;

    ImGuiTextBuffer log_buffer;

    float progress = 0.0f;

    void add_mod(const std::string& name)
    {
        mods.push_back({name, false});
    }

    std::optional<size_t> get_selected_mod()
    {
        for (size_t i = 0; i < mods.size(); ++i)
        {
            if (mods[i].selected)
            {
                return i;
            }
        }
        return std::nullopt;
    }

    void mod_up()
    {
        auto selected = get_selected_mod();
        if (!selected.has_value() || selected.value() == 0)
        {
            return;
        }
        std::swap(mods[selected.value()], mods[selected.value() - 1]);
    }

    void mod_down()
    {
        auto selected = get_selected_mod();
        if (!selected.has_value() || selected.value() == mods.size() - 1)
        {
            return;
        }
        std::swap(mods[selected.value()], mods[selected.value() + 1]);
    }
};

extern RenderData g_render_data;
extern VraData g_vra_data;

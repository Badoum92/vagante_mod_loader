#include <iostream>

#include "graphics.hh"
#include "vra.hh"

GLFWwindow* g_window = nullptr;
RenderData g_render_data;
VraData g_vra_data;

int main(int, char**)
{
    g_window = create_window();

    g_render_data.background = load_texture("assets/background.png");
    g_render_data.browse = load_texture("assets/browse.png");
    g_render_data.unpack = load_texture("assets/unpack.png");
    g_render_data.repack = load_texture("assets/repack.png");
    g_render_data.up = load_texture("assets/up.png");
    g_render_data.down = load_texture("assets/down.png");
    g_render_data.load_mods = load_texture("assets/load_mods.png");
    g_render_data.progress_empty = load_texture("assets/progress_empty.png");
    g_render_data.progress_full = load_texture("assets/progress_full.png");

    try
    {
        while (!should_close(g_window))
        {
            render(g_render_data, g_vra_data);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Uncaught exception: " << e.what() << "\n";
    }

    destroy_window(g_window);
}

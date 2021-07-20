#include "graphics.hh"

int main(int, char**)
{
    auto window = create_window();

    RenderData render_data = {};
    render_data.texture = load_texture("file.png");

    while (!should_close(window))
    {
        render(render_data);
        update_window(window);
    }

    destroy_window(window);
}

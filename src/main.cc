#include "graphics.hh"

int main(int, char**)
{
    auto window = create_window();

    while (!should_close(window))
    {
        render();
        update_window(window);
    }

    destroy_window(window);
}

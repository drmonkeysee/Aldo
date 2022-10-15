//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include <SDL2/SDL.h>

namespace
{

//
// UI Interface Implementation
//

[[nodiscard("error code")]]
int init_ui() noexcept
{
    SDL_Log("SDL INIT UI");
    return 0;
}

void tick_start(control* appstate, const console_state* snapshot) noexcept
{
    SDL_Log("SDL TICK START");
}

void tick_end(control* appstate) noexcept
{
    SDL_Log("SDL TICK END");
}

int pollinput() noexcept
{
    SDL_Log("SDL POLL INPUT");
    return 0;
}

void render_ui(const control* appstate, const console_state* snapshot) noexcept
{
    SDL_Log("SDL RENDER UI");
}

void cleanup_ui(const control* appstate,
                const console_state* snapshot) noexcept
{
    SDL_Log("SDL CLEANUP UI");
}

}

//
// Public Interface
//

int aldo::ui_sdl_init(ui_interface* ui) noexcept
{
    const auto err = init_ui();
    if (err == 0) {
        *ui = {
            tick_start,
            tick_end,
            pollinput,
            render_ui,
            cleanup_ui,
        };
    }
    return err;
}

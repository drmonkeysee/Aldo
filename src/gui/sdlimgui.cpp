//
//  sdlimgui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/11/22.
//

#include "sdlimgui.hpp"

#include <SDL2/SDL.h>

#include <cassert>

namespace
{

//
// UI Interface Implementation
//

int init_ui() noexcept
{
    SDL_Log("SDLIMGUI INIT");
    return 0;
}

void tick_start(control* appstate, const console_state* snapshot) noexcept
{
    assert(appstate != nullptr);
    assert(snapshot != nullptr);

    SDL_Log("SDLIMGUI TICK START");
}

void tick_end(control* appstate) noexcept
{
    assert(appstate != nullptr);

    SDL_Log("SDLIMGUI TICK END");
}

int pollinput() noexcept
{
    SDL_Log("SDLIMGUI POLL INPUT");
    return 0;
}

void render_ui(const control* appstate, const console_state* snapshot) noexcept
{
    assert(appstate != nullptr);
    assert(snapshot != nullptr);

    SDL_Log("SDLIMGUI REFRESH");
}

void cleanup_ui(const control* appstate,
                const console_state* snapshot) noexcept
{
    assert(appstate != nullptr);
    assert(snapshot != nullptr);

    SDL_Log("SDLIMGUI CLEANUP");
}

}

//
// Public Interface
//

int aldo::sdlimgui_init(ui_interface* ui) noexcept
{
    assert(ui != nullptr);

    const auto result = init_ui();
    if (result == 0) {
        *ui = {
            tick_start,
            tick_end,
            pollinput,
            render_ui,
            cleanup_ui,
        };
    }
    return result;
}

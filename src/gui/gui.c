//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "ui.h"
#include "uisdl.hpp"

#include <SDL2/SDL.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

static int run_emu(const struct gui_platform *platform)
{
    const int err = ui_sdl_runloop(platform);
    if (err < 0) {
        SDL_Log("UI run failure (%d): %s", err, ui_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//
// Public Interface
//

int gui_run(const struct gui_platform *platform)
{
    assert(platform != NULL);

    return run_emu(platform);
}

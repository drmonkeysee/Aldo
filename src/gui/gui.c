//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "snapshot.h"
#include "ui.h"
#include "uisdl.hpp"

#include <SDL2/SDL.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

static int init_ui(const struct gui_platform *platform, ui_loop **loop)
{
    return ui_sdl_init(platform, loop);
}

static int sdl_demo(const struct gui_platform *platform)
{
    int result = EXIT_SUCCESS;
    struct console_state snapshot = {0};

    ui_loop *loop;
    const int err = init_ui(platform, &loop);
    if (err < 0) {
        SDL_Log("UI init failure (%d): %s", err, ui_errstr(err));
        result = EXIT_FAILURE;
        goto exit_console;
    }

    loop(NULL, &snapshot);
exit_console:
    snapshot_clear(&snapshot);
    return result;
}

//
// Public Interface
//

int gui_run(const struct gui_platform *platform)
{
    assert(platform != NULL);

    return sdl_demo(platform);
}

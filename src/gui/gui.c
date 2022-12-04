//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "debug.h"
#include "nes.h"
#include "snapshot.h"
#include "ui.h"
#include "uisdl.hpp"

#include <SDL2/SDL.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

static int run_emu(const struct gui_platform *platform)
{
    debugctx *dbg = debug_new();
    if (!dbg) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize debugger!");
        return EXIT_FAILURE;
    }

    nes *console = nes_new(dbg, false, NULL);
    if (!console) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize console!");
        debug_free(dbg);
        return EXIT_FAILURE;
    }
    nes_powerup(console, NULL, false);

    struct console_state snapshot;
    nes_snapshot(console, &snapshot);
    const int err = ui_sdl_runloop(platform, dbg, console, &snapshot);
    // NOTE: ui loop takes ownership of these two,
    // even in the event of UI init failure.
    console = NULL;
    dbg = NULL;
    int result = EXIT_SUCCESS;
    if (err < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "UI run failure (%d): %s", err, ui_errstr(err));
        result = EXIT_FAILURE;
    }
    snapshot_clear(&snapshot);
    return result;
}

//
// Public Interface
//

int gui_run(const struct gui_platform *platform)
{
    assert(platform != NULL);

    return run_emu(platform);
}

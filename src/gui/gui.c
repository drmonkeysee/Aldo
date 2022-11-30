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
    if (!dbg) return EXIT_FAILURE;

    int result = EXIT_SUCCESS;
    nes *console = nes_new(dbg, false, NULL);
    if (!console) {
        result = EXIT_FAILURE;
        goto exit_debug;
    }
    nes_powerup(console, NULL, false);

    struct console_state snapshot;
    nes_snapshot(console, &snapshot);
    const int err = ui_sdl_runloop(platform, console, &snapshot);
    if (err < 0) {
        SDL_Log("UI run failure (%d): %s", err, ui_errstr(err));
        result = EXIT_FAILURE;
    }
    snapshot_clear(&snapshot);
    nes_free(console);
    console = NULL;
exit_debug:
    debug_free(dbg);
    dbg = NULL;
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

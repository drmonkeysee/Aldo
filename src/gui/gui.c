//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "debug.h"
#include "guiplatform.h"
#include "nes.h"
#include "ui.h"
#include "uisdl.hpp"

#include <SDL2/SDL.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int run_emu(const struct gui_platform *platform)
{
    // NOTE: create initial debugger and console objects before launching
    // UI loop cuz if we can't get this far then bail immediately.
    aldo_debugger *dbg = aldo_debug_new();
    if (!dbg) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize debugger (%d): %s", errno,
                        strerror(errno));
        return EXIT_FAILURE;
    }

    aldo_nes *console = aldo_nes_new(dbg, false, NULL);
    if (!console) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize console (%d): %s", errno,
                        strerror(errno));
        aldo_debug_free(dbg);
        return EXIT_FAILURE;
    }
    aldo_nes_powerup(console, NULL, false);

    int err = ui_sdl_runloop(platform, dbg, console);
    // NOTE: ui loop takes ownership of these two,
    // even in the event of UI init failure.
    console = NULL;
    dbg = NULL;
    if (err < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "UI run failure (%d): %s", err, aldo_ui_errstr(err));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//
// MARK: - Public Interface
//

int gui_run(const struct gui_platform *platform)
{
    assert(platform != NULL);

    return run_emu(platform);
}

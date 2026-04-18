//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "cart.h"
#include "console.h"
#include "debug.h"
#include "guiplatform.h"
#include "ui.h"
#include "uisdl.hpp"

#include <SDL3/SDL.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int run_emu(const struct gui_platform *platform)
{
    // Create initial debugger, cart, and console objects before launching
    // UI loop cuz if we can't get this far then bail immediately.
    auto dbg = aldo_debug_new();
    if (!dbg) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize debugger (%d): %s", errno,
                        strerror(errno));
        return EXIT_FAILURE;
    }

    auto result = EXIT_SUCCESS;
    aldo_cart *cart = nullptr;
    aldo_console *console = nullptr;

    auto err = aldo_cart_create(&cart, nullptr);
    if (err < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize blank cart (%d): %s", err,
                        aldo_cart_errstr(err));
        result = EXIT_FAILURE;
        goto cleanup;
    }

    if (!aldo_console_poweron(&console, cart, dbg, nullptr, false)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Unable to initialize console (%d): %s", errno,
                        strerror(errno));
        result = EXIT_FAILURE;
        goto cleanup;
    }

    err = ui_sdl_runloop(platform, dbg, cart, console);
    // ui loop takes ownership of these, even in the event of UI init failure
    console = nullptr;
    cart = nullptr;
    dbg = nullptr;
    if (err < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "UI run failure (%d): %s",
                        err, aldo_ui_errstr(err));
        result = EXIT_FAILURE;
    }
cleanup:
    aldo_console_free(console);
    aldo_cart_free(cart);
    aldo_debug_free(dbg);
    return result;
}

//
// MARK: - Public Interface
//

int gui_run(const struct gui_platform *platform)
{
    assert(platform != nullptr);

    return run_emu(platform);
}

//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "argparse.h"
#include "control.h"
#include "snapshot.h"
#include "ui.h"
#include "uiimgui.hpp"
#include "uisdl.hpp"

#include <SDL2/SDL.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static int init_ui(const struct control *appstate,
                   const struct gui_platform *platform,
                   struct ui_interface *ui)
{
    return appstate->batch
            ? ui_batch_init(ui)
            : ui_sdl_init(ui, appstate, platform);
}

static void handle_input(SDL_Event *ev, struct control *appstate)
{
    while (SDL_PollEvent(ev)) {
        imgui_handle_input(ev);
        if (ev->type == SDL_QUIT) {
            appstate->running = false;
        }
    }
}

static void update(struct bounce *bouncer)
{
    const int halfdim = bouncer->dim / 2;
    if (bouncer->pos.x - halfdim < 0
        || bouncer->pos.x + halfdim > bouncer->bounds.x) {
        bouncer->velocity.x *= -1;
    }
    if (bouncer->pos.y - halfdim < 0
        || bouncer->pos.y + halfdim > bouncer->bounds.y) {
        bouncer->velocity.y *= -1;
    }
    bouncer->pos.x += bouncer->velocity.x;
    bouncer->pos.y += bouncer->velocity.y;
}

static int sdl_demo(struct control *appstate,
                    const struct gui_platform *platform)
{
    appstate->bouncer = (struct bounce){
        {256, 240},
        {256 / 2, 240 / 2},
        {1, 1},
        50,
    };

    struct console_state snapshot = {0};
    struct ui_interface ui;
    const int err = init_ui(appstate, platform, &ui);
    if (err < 0) {
        SDL_Log("UI init failure (%d): %s", err, ui_errstr(err));
        return EXIT_FAILURE;
    }
    SDL_Event ev;
    do {
        ui.tick_start(appstate, &snapshot);
        ui.pollinput();
        handle_input(&ev, appstate);
        update(&appstate->bouncer);
        ui.render(appstate, &snapshot);
        ui.tick_end(appstate);
    } while (appstate->running);

    ui.cleanup(appstate, &snapshot);

    return EXIT_SUCCESS;
}

//
// Public Interface
//

int gui_run(int argc, char *argv[argc+1],
            const struct gui_platform *restrict platform)
{
    assert(platform != NULL);

    struct control appstate;
    if (!argparse_parse(&appstate, argc, argv)) return EXIT_FAILURE;

    const int result = sdl_demo(&appstate, platform);
    argparse_cleanup(&appstate);
    return result;
}

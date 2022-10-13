//
//  uisdl.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/11/22.
//

#include "ui.h"

#include "control.h"
#include "sdlimgui.hpp"
#include "snapshot.h"

#include <SDL2/SDL.h>

#include <assert.h>
#include <stddef.h>

//
// UI Interface Implementation
//

static void tick_start(struct control *appstate,
                       const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    SDL_Log("UISDL TICK START");
}

static void tick_end(struct control *appstate)
{
    assert(appstate != NULL);

    SDL_Log("UISDL TICK END");
}

static int pollinput(void)
{
    SDL_Log("UISDL POLL INPUT");
    return 0;
}

static void render_ui(const struct control *appstate,
                      const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    SDL_Log("UISDL RENDER");
}

static void cleanup_ui(const struct control *appstate,
                       const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    SDL_Log("UISDL CLEANUP");
}

//
// Public Interface
//

int ui_sdl_init(struct ui_interface *ui)
{
    assert(ui != NULL);

    const int result = sdlimgui_init();
    if (result == 0) {
        *ui = (struct ui_interface){
            tick_start,
            tick_end,
            pollinput,
            render_ui,
            cleanup_ui,
        };
    }
    return result;
}

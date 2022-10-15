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

static SDL_Window *restrict Window;
static SDL_Renderer *restrict Renderer;

enum guicleanup {
    GUI_CLEANUP_ALL,
    GUI_CLEANUP_RENDERER,
    GUI_CLEANUP_WINDOW,
    GUI_CLEANUP_SDL,
};

static int ui_init(const struct control *appstate, struct ui_interface *ui)
{
    return appstate->batch ? ui_batch_init(ui) : ui_sdl_init(ui);
}

static void ui_cleanup(enum guicleanup status)
{
    switch (status) {
    case GUI_CLEANUP_ALL:
        imgui_cleanup();
        // NOTE: fallthrough
    case GUI_CLEANUP_RENDERER:
        SDL_DestroyRenderer(Renderer);
        Renderer = NULL;
        // NOTE: fallthrough
    case GUI_CLEANUP_WINDOW:
        SDL_DestroyWindow(Window);
        Window = NULL;
        // NOTE: fallthrough
    default:
        SDL_Quit();
    }
}

static bool demo_init(const struct gui_platform *platform)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL initialization failure: %s", SDL_GetError());
        return false;
    }

    const bool hidpi = platform->is_hidpi();
    SDL_Log("HIDPI: %d", hidpi);
    const char *const name = platform->appname();
    Window = SDL_CreateWindow(name ? name : "DisplayNameErr",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              1280, 800, hidpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0);
    if (!Window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        ui_cleanup(GUI_CLEANUP_SDL);
        return false;
    }

    Renderer = SDL_CreateRenderer(Window, -1,
                                  SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC
                                  | SDL_RENDERER_TARGETTEXTURE);
    if (!Renderer) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s",
                        SDL_GetError());
        ui_cleanup(GUI_CLEANUP_WINDOW);
        return false;
    }
    const float render_scale_factor = platform->render_scale_factor(Window);
    SDL_RenderSetScale(Renderer, render_scale_factor, render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(Renderer, &info);
    SDL_Log("Render info: %s (%04X) (x%.1f)", info.name, info.flags,
            render_scale_factor);

    if (!imgui_init(Window, Renderer)) {
        ui_cleanup(GUI_CLEANUP_RENDERER);
        return false;
    }

    return true;
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
    if (!demo_init(platform)) return EXIT_FAILURE;

    struct bounce bouncer = {
        {256, 240},
        {256 / 2, 240 / 2},
        {1, 1},
        50,
    };

    struct console_state snapshot = {0};

    struct ui_interface ui;
    if (ui_init(appstate, &ui) < 0) {
        SDL_Log("BAD UI");
        return EXIT_FAILURE;
    }
    SDL_Event ev;
    bool show_demo = false;
    do {
        ui.tick_start(appstate, &snapshot);
        ui.pollinput();
        handle_input(&ev, appstate);
        update(&bouncer);
        ui.render(appstate, &snapshot);
        ui.tick_end(appstate);
        imgui_render(&bouncer, &show_demo, Window, Renderer);
    } while (appstate->running);

    ui_cleanup(GUI_CLEANUP_ALL);
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

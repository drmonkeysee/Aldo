//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "argparse.h"
#include "control.h"
#include "uiimgui.hpp"

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

static bool ui_init(const struct gui_platform *platform)
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
                                  | SDL_RENDERER_PRESENTVSYNC);
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

static int sdl_demo(struct control *appstate,
                    const struct gui_platform *platform)
{
    if (!ui_init(platform)) return EXIT_FAILURE;

    static const SDL_Point boxsize = {256, 240};
    SDL_Point winsize;
    SDL_GetWindowSize(Window, &winsize.x, &winsize.y);
    const SDL_Rect box = {
        (winsize.x - boxsize.x) / 2,
        (winsize.y - boxsize.y) / 2,
        boxsize.x,
        boxsize.y,
    };
    static const int bounce_dim = 50;
    SDL_Rect bouncer = {
        (winsize.x - bounce_dim) / 2,
        (winsize.y - bounce_dim) / 2,
        bounce_dim,
        bounce_dim,
    };
    SDL_Point velocity = {1, 1};
    SDL_Event ev;
    bool show_demo = false;
    do {
        handle_input(&ev, appstate);

        bouncer.x += velocity.x;
        bouncer.y += velocity.y;
        if (bouncer.x < box.x || (bouncer.x + bouncer.w) > (box.x + box.w)) {
            velocity.x *= -1;
        }
        if (bouncer.y < box.y || (bouncer.y + bouncer.h) > (box.y + box.h)) {
            velocity.y *= -1;
        }

        imgui_render(&show_demo);

        SDL_SetRenderDrawColor(Renderer, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(Renderer);

        SDL_SetRenderDrawColor(Renderer, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(Renderer, &box);

        SDL_SetRenderDrawColor(Renderer, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(Renderer, &bouncer);

        SDL_SetRenderDrawColor(Renderer, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(Renderer, 100, 80, 250, 500);

        // TODO: drop this extra step once demo render is rolled into imgui
        imgui_commit_render();
        SDL_RenderPresent(Renderer);
    } while (appstate->running);

    ui_cleanup(GUI_CLEANUP_ALL);

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

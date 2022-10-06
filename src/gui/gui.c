//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "argparse.h"
#include "control.h"

#include <SDL2/SDL.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static SDL_Window *restrict Window;
static SDL_Renderer *restrict Renderer;

enum guicleanup {
    GUI_CLEANUP_ALL,
    GUI_CLEANUP_WINDOW,
    GUI_CLEANUP_SDL,
};

static void ui_cleanup(enum guicleanup status)
{
    switch (status) {
    case GUI_CLEANUP_ALL:
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
    SDL_Log("Is HIDPI: %d", hidpi);
    Window = SDL_CreateWindow("Aldo",
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

    return true;
}

static void handle_input(SDL_Event *ev, struct control *appstate)
{
    while (SDL_PollEvent(ev)) {
        //ImGui_ImplSDL2_ProcessEvent(&ev);
        if (ev->type == SDL_QUIT) {
            appstate->running = false;
        }
    }
}

static int sdl_demo(struct control *appstate,
                    const struct gui_platform *platform)
{
    if (!ui_init(platform)) return EXIT_FAILURE;


    /*aldo::MediaRuntime runtime{options, windowSize};

    ImGui::StyleColorsDark();*/

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
    //auto showDemo = false;
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

        /*ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Support")) {
                ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }*/

        /*if (showDemo) {
            ImGui::ShowDemoWindow();
        }
        ImGui::Begin("First Window");
        ImGui::Text("Hello From Aldo+Dear ImGui");
        ImGui::End();

        ImGui::Render();*/

        SDL_SetRenderDrawColor(Renderer, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(Renderer);

        SDL_SetRenderDrawColor(Renderer, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(Renderer, &box);

        SDL_SetRenderDrawColor(Renderer, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(Renderer, &bouncer);

        SDL_SetRenderDrawColor(Renderer, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(Renderer, 100, 80, 250, 500);

        //ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(Renderer);
    } while (appstate->running);

    ui_cleanup(GUI_CLEANUP_ALL);

    return EXIT_SUCCESS;
}

//
// Public Interface
//

int gui_run(int argc, char *argv[argc+1], const struct gui_platform *platform)
{
    assert(platform != NULL);

    struct control appstate;
    if (!argparse_parse(&appstate, argc, argv)) return EXIT_FAILURE;

    const int result = sdl_demo(&appstate, platform);
    argparse_cleanup(&appstate);
    return result;
}

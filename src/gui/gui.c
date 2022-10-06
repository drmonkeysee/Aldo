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
#include <stddef.h>
#include <stdlib.h>

static int sdl_demo(struct control *appstate,
                    const struct gui_platform *platform)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL initialization failure: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    static const SDL_Point winsize = {1280, 800};
    SDL_Window *const win = SDL_CreateWindow("Aldo",
                                             SDL_WINDOWPOS_CENTERED,
                                             SDL_WINDOWPOS_CENTERED,
                                             winsize.x, winsize.y,
                                             platform->is_hidpi()
                                                ? SDL_WINDOW_ALLOW_HIGHDPI
                                                : 0);
    if (!win) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        goto exit_sdl;
    }

    SDL_Renderer *const ren = SDL_CreateRenderer(win, -1,
                                                 SDL_RENDERER_ACCELERATED
                                                 | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s",
                        SDL_GetError());
        goto exit_window;
    }
    const float render_scale_factor = platform->render_scale_factor(win);
    SDL_RenderSetScale(ren, render_scale_factor, render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(ren, &info);
    SDL_Log("Render name: %s (%04X)", info.name, info.flags);

    /*aldo::MediaRuntime runtime{options, windowSize};

    ImGui::StyleColorsDark();*/

    static const SDL_Point boxsize = {256, 240};
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
        while (SDL_PollEvent(&ev)) {
            //ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) {
                appstate->running = false;
            }
        }

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

        SDL_SetRenderDrawColor(ren, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(ren, &box);

        SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(ren, &bouncer);

        SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(ren, 100, 80, 250, 500);

        //ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(ren);
    } while (appstate->running);

    SDL_DestroyRenderer(ren);
exit_window:
    SDL_DestroyWindow(win);
exit_sdl:
    SDL_Quit();

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

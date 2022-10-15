//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL2/SDL.h>

namespace
{

SDL_Window* Window;
SDL_Renderer* Renderer;
SDL_Texture* BouncerTexture;
bool ShowDemo;

enum guicleanup {
    GUI_CLEANUP_ALL,
    GUI_CLEANUP_BOUNCER,
    GUI_CLEANUP_RENDERER,
    GUI_CLEANUP_WINDOW,
    GUI_CLEANUP_SDL,
};

// TODO: this all needs to be RAII
void ui_cleanup(guicleanup status) noexcept
{
    switch (status) {
    case GUI_CLEANUP_ALL:
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        // NOTE: fallthrough
    case GUI_CLEANUP_BOUNCER:
        SDL_DestroyTexture(BouncerTexture);
        BouncerTexture = nullptr;
        // NOTE: fallthrough
    case GUI_CLEANUP_RENDERER:
        SDL_DestroyRenderer(Renderer);
        Renderer = nullptr;
        // NOTE: fallthrough
    case GUI_CLEANUP_WINDOW:
        SDL_DestroyWindow(Window);
        Window = nullptr;
        // NOTE: fallthrough
    default:
        SDL_Quit();
    }
}

void render_bouncer(const bounce& bouncer) noexcept
{
    SDL_SetRenderTarget(Renderer, BouncerTexture);
    SDL_SetRenderDrawColor(Renderer, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(Renderer);

    SDL_SetRenderDrawColor(Renderer, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(Renderer, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(Renderer, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
    const int halfDim = bouncer.dim / 2;
    const SDL_Rect pos{
        bouncer.pos.x - halfDim,
        bouncer.pos.y - halfDim,
        bouncer.dim,
        bouncer.dim,
    };
    SDL_RenderFillRect(Renderer, &pos);
    SDL_SetRenderTarget(Renderer, nullptr);

    ImGui::Begin("Bouncer");
    const ImVec2 sz{(float)bouncer.bounds.x, (float)bouncer.bounds.y};
    ImGui::Image(BouncerTexture, sz);
    ImGui::End();
}

//
// UI Interface Implementation
//

[[nodiscard("error code")]]
int init_ui(const control& appstate, const gui_platform& platform) noexcept
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL initialization failure: %s", SDL_GetError());
        return UI_ERR_LIBINIT;
    }

    const bool hidpi = platform.is_hidpi();
    SDL_Log("HIDPI: %d", hidpi);
    const char *const name = platform.appname();
    Window = SDL_CreateWindow(name ? name : "DisplayNameErr",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              1280, 800, hidpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0);
    if (!Window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        ui_cleanup(GUI_CLEANUP_SDL);
        return UI_ERR_LIBINIT;
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
        return UI_ERR_LIBINIT;
    }
    const float render_scale_factor = platform.render_scale_factor(Window);
    SDL_RenderSetScale(Renderer, render_scale_factor, render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(Renderer, &info);
    SDL_Log("Render info: %s (%04X) (x%.1f)", info.name, info.flags,
            render_scale_factor);

    BouncerTexture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET,
                                       appstate.bouncer.bounds.x,
                                       appstate.bouncer.bounds.y);
    if (!BouncerTexture) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Bouncer texture creation failure: %s",
                        SDL_GetError());
        ui_cleanup(GUI_CLEANUP_RENDERER);
        return UI_ERR_LIBINIT;
    }

    if (!IMGUI_CHECKVERSION()) {
        ui_cleanup(GUI_CLEANUP_BOUNCER);
        return UI_ERR_LIBINIT;
    }

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(Window, Renderer);
    ImGui_ImplSDLRenderer_Init(Renderer);

    ImGui::StyleColorsDark();

    return 0;
}

void tick_start(control*, const console_state*) noexcept
{
    // TODO: do nothing
}

void tick_end(control*) noexcept
{
    // TODO: do nothing
}

int pollinput() noexcept
{
    // TODO: need to move SDL_Event handling here
    return 0;
}

void render_ui(const control* appstate, const console_state*) noexcept
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Support")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &ShowDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (ShowDemo) {
        ImGui::ShowDemoWindow();
    }
    ImGui::Begin("First Window");
    ImGui::Text("Hello From Aldo+Dear ImGui");
    ImGui::End();

    render_bouncer(appstate->bouncer);

    ImGui::Render();
    SDL_SetRenderDrawColor(Renderer, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(Renderer);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(Renderer);
}

void cleanup_ui(const control*, const console_state*) noexcept
{
    ui_cleanup(GUI_CLEANUP_ALL);
}

}

//
// Public Interface
//

int aldo::ui_sdl_init(ui_interface* ui, const control* appstate,
                      const gui_platform* platform) noexcept
{
    const auto err = init_ui(*appstate, *platform);
    if (err == 0) {
        *ui = {
            tick_start,
            tick_end,
            pollinput,
            render_ui,
            cleanup_ui,
        };
    }
    return err;
}

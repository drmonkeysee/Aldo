//
//  uiimgui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/6/22.
//

#include "uiimgui.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

namespace
{

SDL_Texture* BouncerTexture;

void renderBouncer(const aldo::bounce* bouncer, SDL_Renderer* ren) noexcept
{
    SDL_SetRenderTarget(ren, BouncerTexture);
    SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(ren, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
    const int halfDim = bouncer->dim / 2;
    const SDL_Rect pos{
        bouncer->pos.x - halfDim,
        bouncer->pos.y - halfDim,
        bouncer->dim,
        bouncer->dim,
    };
    SDL_RenderFillRect(ren, &pos);
    SDL_SetRenderTarget(ren, nullptr);

    ImGui::Begin("Bouncer");
    const ImVec2 sz{(float)bouncer->bounds.x, (float)bouncer->bounds.y};
    ImGui::Image(BouncerTexture, sz);
    ImGui::End();
}

}

bool aldo::imgui_init(SDL_Window* win, SDL_Renderer* ren) noexcept
{
    if (!IMGUI_CHECKVERSION()) return false;

    BouncerTexture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET, 256, 240);
    if (!BouncerTexture) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Bouncer texture creation failure: %s",
                        SDL_GetError());
        return false;
    }

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer_Init(ren);

    ImGui::StyleColorsDark();

    return true;
}

void aldo::imgui_handle_input(SDL_Event* ev) noexcept
{
    ImGui_ImplSDL2_ProcessEvent(ev);
}

void aldo::imgui_render(const aldo::bounce* bouncer, bool* show_demo,
                        SDL_Window*, SDL_Renderer* ren) noexcept
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Support")) {
            ImGui::MenuItem("ImGui Demo", nullptr, show_demo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (*show_demo) {
        ImGui::ShowDemoWindow();
    }
    ImGui::Begin("First Window");
    ImGui::Text("Hello From Aldo+Dear ImGui");
    ImGui::End();

    renderBouncer(bouncer, ren);

    ImGui::Render();
    SDL_SetRenderDrawColor(ren, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(ren);
}

void aldo::imgui_cleanup() noexcept
{
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(BouncerTexture);
    BouncerTexture = nullptr;
}

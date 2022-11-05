//
//  render.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#include "render.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

//
// Public Interface
//

aldo::RenderFrame::RenderFrame(aldo::viewstate& s,
                               const aldo::MediaRuntime& r) noexcept
: state{s}, runtime{r}
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

aldo::RenderFrame::~RenderFrame()
{
    const auto ren = runtime.renderer();
    ImGui::Render();
    SDL_SetRenderDrawColor(ren, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(ren);
}

void aldo::RenderFrame::render() const noexcept
{
    renderMainMenu();
    renderBouncer();
    if (state.showDemo) {
        ImGui::ShowDemoWindow();
    }
}

//
// Private Interface
//

void aldo::RenderFrame::renderMainMenu() const noexcept
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Aldo")) {
            ImGui::MenuItem("About");
            ImGui::MenuItem("ImGui Demo", nullptr, &state.showDemo);
            ImGui::MenuItem("Quit");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void aldo::RenderFrame::renderBouncer() const noexcept
{
    const auto ren = runtime.renderer();
    const auto tex = runtime.bouncerTexture();
    SDL_SetRenderTarget(ren, tex);
    SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff,SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(ren, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);

    const auto& bouncer = state.bouncer;
    const auto fulldim = bouncer.halfdim * 2;
    const SDL_Rect pos{
        bouncer.pos.x - bouncer.halfdim,
        bouncer.pos.y - bouncer.halfdim,
        fulldim,
        fulldim,
    };
    SDL_RenderFillRect(ren, &pos);
    SDL_SetRenderTarget(ren, nullptr);

    ImGui::Begin("Bouncer");
    const ImVec2 sz{(float)bouncer.bounds.x, (float)bouncer.bounds.y};
    ImGui::Image(tex, sz);
    ImGui::End();
}

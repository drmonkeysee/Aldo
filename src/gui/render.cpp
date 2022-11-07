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

#include <cstddef>
#include <cstdint>

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
    renderCpu();
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
    if (ImGui::Begin("Bouncer")) {
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

        const ImVec2 sz{(float)bouncer.bounds.x, (float)bouncer.bounds.y};
        ImGui::Image(tex, sz);
    }
    ImGui::End();
}

void aldo::RenderFrame::renderCpu() const noexcept
{
    if (ImGui::Begin("CPU")) {
        if (ImGui::CollapsingHeader("Registers")) {
            ImGui::BeginGroup();
            {
                ImGui::TextUnformatted("A:");
                ImGui::SameLine();
                ImGui::TextUnformatted("FF");
                ImGui::TextUnformatted("X:");
                ImGui::SameLine();
                ImGui::TextUnformatted("FF");
                ImGui::TextUnformatted("Y:");
                ImGui::SameLine();
                ImGui::TextUnformatted("FF");
            }
            ImGui::EndGroup();
            ImGui::SameLine(100);
            ImGui::BeginGroup();
            {
                ImGui::TextUnformatted("PC:");
                ImGui::SameLine();
                ImGui::TextUnformatted("FFFF");
                ImGui::TextUnformatted(" S:");
                ImGui::SameLine();
                ImGui::TextUnformatted("FF");
                ImGui::TextUnformatted(" P:");
                ImGui::SameLine();
                ImGui::TextUnformatted("FF");
            }
            ImGui::EndGroup();
        }

        static constexpr char flags[] = {
            'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C',
        };
        static constexpr auto
            flagOn = IM_COL32(255, 255, 0, 255),
            flagOff = IM_COL32(67, 57, 54, 255),
            textOn = IM_COL32_BLACK,
            textOff = IM_COL32_WHITE;
        static constexpr auto radius = 10.0f;
        static constexpr std::uint8_t status = 0x34;
        if (ImGui::CollapsingHeader("Flags")) {
            const auto pos = ImGui::GetCursorScreenPos();
            ImVec2 center{pos.x + radius, pos.y + radius};
            const auto
                fontSz = ImGui::GetFontSize(),
                xOffset = fontSz / 4,
                yOffset = fontSz / 2;
            const auto drawList = ImGui::GetWindowDrawList();
            for (std::size_t i = 0; i < sizeof flags; ++i) {
                ImU32 fill, text;
                if (status & (1 << (sizeof flags - 1 - i))) {
                    fill = flagOn;
                    text = textOn;
                } else {
                    fill = flagOff;
                    text = textOff;
                }
                drawList->AddCircleFilled(center, radius, fill);
                drawList->AddText({center.x - xOffset, center.y - yOffset},
                                  text, flags + i, flags + i + 1);
                center.x += 25;
            }
        }
    }
    ImGui::End();
}

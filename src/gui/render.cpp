//
//  render.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#include "render.hpp"

#include "mediaruntime.hpp"
#include "viewstate.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <array>
#include <random>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{

// NOTE: not actually noexcept
auto fake_ram(std::array<int, 0x800>& testRam) noexcept
{
    std::random_device d;
    std::mt19937_64 gen{d()};
    std::uniform_int_distribution dist{0, 255};
    for (auto& c : testRam) {
        c = dist(gen);
    }
}

}

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
    renderHardwareTraits();
    renderBouncer();
    renderCpu();
    renderRam();
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
            if (ImGui::MenuItem("Quit", "Cmd+Q")) {
                state.running = false;
            };
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Bouncer", nullptr, &state.showBouncer);
            ImGui::MenuItem("CPU", nullptr, &state.showCpu);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void aldo::RenderFrame::renderHardwareTraits() const noexcept
{
    if (ImGui::Begin("Hardware Traits")) {
        static int cps = 4;
        static bool halt = false;
        ImGui::TextUnformatted("FPS: 60");
        ImGui::TextUnformatted("Runtime: N.NN");
        ImGui::TextUnformatted("Frames: NN");
        ImGui::TextUnformatted("Cycles: NN");

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cycles/Second");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::DragInt("##cyclesPerSecond", &cps, 1.0f, 1, 100, "%d",
                       ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();

        ImGui::Checkbox("HALT", &halt);
        ImGui::TextUnformatted("Mode");
        ImGui::RadioButton("Cycle", true);
        ImGui::SameLine();
        ImGui::RadioButton("Step", false);
        ImGui::SameLine();
        ImGui::RadioButton("Run", false);

        // TODO: fake toggle button by using on/off flags to adjust colors
        ImGui::TextUnformatted("Signal");
        ImGui::PushStyleColor(ImGuiCol_Button,
                              IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE));
        ImGui::Button("IRQ");
        ImGui::SameLine();
        ImGui::Button("NMI");
        ImGui::SameLine();
        ImGui::Button("RES");
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

void aldo::RenderFrame::renderBouncer() const noexcept
{
    if (!state.showBouncer) return;

    if (ImGui::Begin("Bouncer", &state.showBouncer)) {
        const auto ren = runtime.renderer();
        const auto tex = runtime.bouncerTexture();
        SDL_SetRenderTarget(ren, tex);
        SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
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
    if (!state.showCpu) return;

    if (ImGui::Begin("CPU", &state.showCpu)) {
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
            ImGui::SameLine(125);
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
            flagOn = IM_COL32(0xff, 0xff, 0, SDL_ALPHA_OPAQUE),
            flagOff = IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE),
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
            ImGui::Dummy({radius * 2, radius * 2});
        }

        if (ImGui::CollapsingHeader("PRG @ PC")) {
            static int selected = -1;
            char buf[28];
            std::uint16_t addr = 0x804f;
            for (int i = 0; i < 7; ++i) {
                std::snprintf(buf, sizeof buf, "%04X: FF FF FF *ISC $FFFF,X",
                              addr);
                if (ImGui::Selectable(buf, selected == i)) {
                    selected = i;
                }
                addr += 3;
            }
        }

        if (ImGui::CollapsingHeader("Vectors")) {
            ImGui::TextUnformatted("FFFA: 04 80     NMI $8004");
            ImGui::TextUnformatted("FFFC: 00 80     RES $8000");
            ImGui::TextUnformatted("FFFE: 04 80     IRQ $8004");
        }
    }
    ImGui::End();
}

void aldo::RenderFrame::renderRam() const noexcept
{
    if (ImGui::Begin("RAM")) {
        static constexpr auto width = 16, cols = width + 2;
        if (ImGui::BeginTable("ram", cols, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
            char col[3];
            ImGui::TableSetupColumn("Addr");
            for (auto i = 0; i < width; ++i) {
                snprintf(col, sizeof col, " %01X", i);
                ImGui::TableSetupColumn(col);
            }
            ImGui::TableSetupColumn("ASCII");
            ImGui::TableHeadersRow();

            std::array<int, 0x800> testRam{};
            fake_ram(testRam);
            const auto rowCount = testRam.size() / width;
            std::uint16_t addr = 0;
            for (std::size_t i = 0; i < rowCount; ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%04X", addr);
                for (auto j = 0; j < width; ++j) {
                    const auto val = testRam[(std::size_t)j + (i * width)];
                    ImGui::TableSetColumnIndex(j + 1);
                    ImGui::Text("%02X", val);
                }
                ImGui::TableSetColumnIndex(cols - 1);
                ImGui::TextUnformatted("...\5s.6.%0a.Ab>");
                addr += width;
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

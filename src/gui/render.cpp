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
#include <cstddef>
#include <cstdint>
#include <cstdio>

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
        static bool halt = true, irq, nmi, res;
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
        /*ImGui::PushStyleColor(ImGuiCol_Button,
                              IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE));
        ImGui::Button("IRQ");
        ImGui::SameLine();
        ImGui::Button("NMI");
        ImGui::SameLine();
        ImGui::Button("RES");
        ImGui::PopStyleColor();*/
        ImGui::Checkbox("IRQ", &irq);
        ImGui::SameLine();
        ImGui::Checkbox("NMI", &nmi);
        ImGui::SameLine();
        ImGui::Checkbox("RES", &res);
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
        static constexpr std::uint8_t status = 0x34;
        const auto textSize = ImGui::CalcTextSize("A");
        const auto radius = (textSize.x + textSize.y) / 2.0f;
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

        if (ImGui::CollapsingHeader("Datapath")) {
            ImGui::TextUnformatted("Address:");
            ImGui::SameLine();
            ImGui::TextUnformatted("01FB");
            ImGui::TextUnformatted("Data:");
            ImGui::SameLine();
            ImGui::TextUnformatted("00");
            ImGui::SameLine();
            ImGui::TextUnformatted("R/W");

            ImGui::Separator();

            ImGui::TextUnformatted("Decode:");
            ImGui::SameLine();
            ImGui::TextUnformatted("RTS");
            ImGui::TextUnformatted("adl:");
            ImGui::SameLine();
            ImGui::TextUnformatted("07");
            ImGui::TextUnformatted("adh:");
            ImGui::SameLine();
            ImGui::TextUnformatted("00");
            ImGui::TextUnformatted("adc:");
            ImGui::SameLine();
            ImGui::TextUnformatted("00");
            ImGui::TextUnformatted("t:");
            ImGui::SameLine();
            ImGui::TextUnformatted("2");

            ImGui::Separator();

            static bool activeHi, activeLow = true;
            ImGui::Checkbox("RDY", &activeHi);
            ImGui::SameLine();
            ImGui::Checkbox("SYNC", &activeHi);
            ImGui::SameLine();
            ImGui::Checkbox("R/W", &activeHi);

            ImGui::Checkbox("IRQ", &activeLow);
            ImGui::SameLine();
            ImGui::Checkbox("NMI ", &activeLow);
            ImGui::SameLine();
            ImGui::Checkbox("RES", &activeLow);
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
        static constexpr std::array<int, 0x800> testRam{};
        static constexpr auto tableDim = 16, cols = tableDim + 2;
        static constexpr auto rowCount = testRam.size() / tableDim;
        static constexpr auto tableConfig = ImGuiTableFlags_BordersOuter
                                            | ImGuiTableFlags_BordersV
                                            | ImGuiTableFlags_RowBg
                                            | ImGuiTableFlags_SizingFixedFit
                                            | ImGuiTableFlags_ScrollY;
        const ImVec2 tableSize{
            0, ImGui::GetTextLineHeightWithSpacing() * tableDim,
        };
        if (ImGui::BeginTable("ram", cols, tableConfig, tableSize)) {
            char col[3];
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Addr");
            for (auto i = 0; i < tableDim; ++i) {
                snprintf(col, sizeof col, " %01X", i);
                ImGui::TableSetupColumn(col);
            }
            ImGui::TableSetupColumn("ASCII");
            ImGui::TableHeadersRow();

            std::uint16_t addr = 0;
            for (std::size_t i = 0; i < rowCount; ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%04X", addr);
                for (auto j = 0; j < tableDim; ++j) {
                    const auto val = testRam[(std::size_t)j + (i * tableDim)];
                    ImGui::TableSetColumnIndex(j + 1);
                    ImGui::Text("%02X", val);
                }
                ImGui::TableSetColumnIndex(cols - 1);
                ImGui::TextUnformatted("................");
                addr += tableDim;
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

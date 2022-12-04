//
//  render.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#include "render.hpp"

#include "bytes.h"
#include "cart.h"
#include "ctrlsignal.h"
#include "dis.h"
#include "mediaruntime.hpp"
#include "viewstate.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <locale>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{

constexpr auto display_linestate(bool v) noexcept
{
    return v ? "HI" : "LO";
}

constexpr auto display_signalstate(csig_state s) noexcept
{
    switch (s) {
    case CSGS_PENDING:
        return " (P)";
    case CSGS_DETECTED:
        return " (D)";
    case CSGS_COMMITTED:
        return " (C)";
    case CSGS_SERVICED:
        return " (S)";
    default:
        return "";
    }
}

}

//
// Public Interface
//

aldo::RenderFrame::RenderFrame(const aldo::MediaRuntime& r) noexcept
: runtime{r}
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

void aldo::RenderFrame::render(aldo::viewstate& state,
                               const console_state& snapshot) const noexcept
{
    renderMainMenu(state);
    renderHardwareTraits(state, snapshot);
    renderCart(snapshot);
    renderPrg(snapshot);
    renderBouncer(state);
    renderCpu(state, snapshot);
    renderRam(snapshot);
    if (state.showDemo) {
        ImGui::ShowDemoWindow();
    }
}

//
// Private Interface
//

void aldo::RenderFrame::renderMainMenu(aldo::viewstate& state) const noexcept
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(SDL_GetWindowTitle(runtime.window()))) {
            ImGui::MenuItem("About");
            ImGui::MenuItem("ImGui Demo", nullptr, &state.showDemo);
            if (ImGui::MenuItem("Quit", "Cmd+Q")) {
                state.events.emplace(aldo::Command::quit);
            };
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Cmd+O")) {
                state.queueOpenFile();
            }
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

void aldo::RenderFrame::
renderHardwareTraits(aldo::viewstate& state,
                     const console_state& snapshot) const noexcept
{
    if (ImGui::Begin("Hardware Traits")) {
        static constexpr auto refreshIntervalMs = 250;
        static constinit double displayDtUpdate, refreshDt;
        auto& cyclock = state.clock.cyclock;
        if ((refreshDt += cyclock.frametime_ms) >= refreshIntervalMs) {
            displayDtUpdate = state.clock.dtUpdateMs;
            refreshDt = 0;
        }
        ImGui::TextUnformatted("FPS: 60");
        ImGui::Text("Update dT: %.3f", displayDtUpdate);
        ImGui::Text("Frames: %" PRIu64, cyclock.frames);
        ImGui::Text("Runtime: %.3f", cyclock.runtime);
        ImGui::Text("Cycles: %" PRIu64, cyclock.total_cycles);

        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cycles/Second");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::DragInt("##cyclesPerSecond", &cyclock.cycles_per_sec, 1.0f, 1,
                       100, "%d", ImGuiSliderFlags_AlwaysClamp);

        ImGui::Separator();

        auto halt = !snapshot.lines.ready;
        if (ImGui::Checkbox("HALT", &halt)) {
            state.events.emplace(aldo::Command::halt, halt);
        };

        auto mode = snapshot.mode;
        ImGui::TextUnformatted("Mode");
        if (ImGui::RadioButton("Cycle", mode == CSGM_CYCLE)
            && mode != CSGM_CYCLE) {
            state.events.emplace(aldo::Command::mode, CSGM_CYCLE);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Step", mode == CSGM_STEP)
            && mode != CSGM_STEP) {
            state.events.emplace(aldo::Command::mode, CSGM_STEP);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Run", mode == CSGM_RUN)
            && mode != CSGM_RUN) {
            state.events.emplace(aldo::Command::mode, CSGM_RUN);
        }

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
        // NOTE: interrupt signals are all low-active
        auto irq = !snapshot.lines.irq,
                nmi = !snapshot.lines.nmi,
                res = !snapshot.lines.reset;
        if (ImGui::Checkbox("IRQ", &irq)) {
            state.events.emplace(aldo::Command::interrupt,
                                 aldo::interrupt_event{CSGI_IRQ, irq});
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("NMI", &nmi)) {
            state.events.emplace(aldo::Command::interrupt,
                                 aldo::interrupt_event{CSGI_NMI, nmi});
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("RES", &res)) {
            state.events.emplace(aldo::Command::interrupt,
                                 aldo::interrupt_event{CSGI_RES, res});
        }
    }
    ImGui::End();
}

void
aldo::RenderFrame::renderCart(const console_state& snapshot) const noexcept
{
    if (ImGui::Begin("Cart")) {
        const auto& cart = snapshot.cart;
        ImGui::Text("Name: %s", "NOCART");
        char cartFormat[CART_FMT_SIZE];
        const auto result = cart_format_extname(cart.info, cartFormat);
        ImGui::Text("Format: %s", result > 0 ? cartFormat : "Invalid Format");
    }
    ImGui::End();
}

void aldo::RenderFrame::renderPrg(const console_state& snapshot) const noexcept
{
    if (ImGui::Begin("PRG @ PC")) {
        static constexpr auto instCount = 20;
        static constinit auto selected = -1;
        const auto& prgMem = snapshot.mem;
        auto addr = snapshot.datapath.current_instruction;
        dis_instruction inst{};
        char disasm[DIS_INST_SIZE];
        for (int i = 0; i < instCount; ++i) {
            auto result = dis_parsemem_inst(prgMem.prglength,
                                            prgMem.currprg,
                                            inst.offset + inst.bv.size,
                                            &inst);
            if (result > 0) {
                result = dis_inst(addr, &inst, disasm);
                if (result > 0) {
                    if (ImGui::Selectable(disasm, selected == i)) {
                        selected = i;
                    }
                    addr += inst.bv.size;
                    continue;
                }
            }
            if (result < 0) {
                ImGui::TextUnformatted(dis_errstr(result));
            }
            break;
        }

        if (ImGui::CollapsingHeader("Vectors",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            auto lo = prgMem.vectors[0], hi = prgMem.vectors[1];
            ImGui::Text("%04X: %02X %02X     NMI $%04X", CPU_VECTOR_NMI, lo,
                        hi, bytowr(lo, hi));

            lo = prgMem.vectors[2];
            hi = prgMem.vectors[3];
            const auto& debugger = snapshot.debugger;
            const char* indicator;
            std::uint16_t resVector;
            if (debugger.resvector_override >= 0) {
                indicator = "!";
                resVector = (std::uint16_t)debugger.resvector_override;
            } else {
                indicator = "";
                resVector = bytowr(lo, hi);
            }
            ImGui::Text("%04X: %02X %02X     RES %s$%04X", CPU_VECTOR_RES, lo,
                        hi, indicator, resVector);

            lo = prgMem.vectors[4];
            hi = prgMem.vectors[5];
            ImGui::Text("%04X: %02X %02X     IRQ $%04X", CPU_VECTOR_IRQ, lo,
                        hi, bytowr(lo, hi));
        }
    }
    ImGui::End();
}

void
aldo::RenderFrame::renderBouncer(aldo::viewstate& state) const noexcept
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

void aldo::RenderFrame::renderCpu(aldo::viewstate& state,
                                  const console_state& snapshot) const noexcept
{
    if (!state.showCpu) return;

    if (ImGui::Begin("CPU", &state.showCpu)) {
        if (ImGui::BeginChild("CpuLeft", {200, 0})) {
            const auto& cpu = snapshot.cpu;
            if (ImGui::CollapsingHeader("Registers",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BeginGroup();
                {
                    ImGui::Text("A: %02X", cpu.accumulator);
                    ImGui::Text("X: %02X", cpu.xindex);
                    ImGui::Text("Y: %02X", cpu.yindex);
                }
                ImGui::EndGroup();
                ImGui::SameLine(0, 90);
                ImGui::BeginGroup();
                {
                    ImGui::Text("PC: %04X", cpu.program_counter);
                    ImGui::Text(" S: %02X", cpu.stack_pointer);
                    ImGui::Text(" P: %02X", cpu.status);
                }
                ImGui::EndGroup();
            }

            static constexpr char flags[] = {
                'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C',
            };
            static constexpr auto
                flagOn = IM_COL32(0xff, 0xfc, 0x53, SDL_ALPHA_OPAQUE),
                flagOff = IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE),
                textOn = IM_COL32_BLACK,
                textOff = IM_COL32_WHITE;
            const auto textSize = ImGui::CalcTextSize("A");
            const auto radius = (textSize.x + textSize.y) / 2.0f;
            if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen)) {
                const auto pos = ImGui::GetCursorScreenPos();
                ImVec2 center{pos.x + radius, pos.y + radius};
                const auto
                    fontSz = ImGui::GetFontSize(),
                    xOffset = fontSz / 4,
                    yOffset = fontSz / 2;
                const auto drawList = ImGui::GetWindowDrawList();
                for (std::size_t i = 0; i < sizeof flags; ++i) {
                    ImU32 fill, text;
                    if (cpu.status & (1 << (sizeof flags - 1 - i))) {
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
                ImGui::Dummy({0, radius * 2});
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("CpuRight")) {
            if (ImGui::CollapsingHeader("Datapath",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                const auto& datapath = snapshot.datapath;
                const auto& lines = snapshot.lines;

                ImGui::Text("Address Bus: %04X", datapath.addressbus);
                const char* dataStr;
                char dataHex[3];
                if (datapath.busfault) {
                    dataStr = "FLT";
                } else {
                    snprintf(dataHex, sizeof dataHex,
                             "%02X", datapath.databus);
                    dataStr = dataHex;
                }
                ImGui::Text("Data Bus: %s %c", dataStr,
                            lines.readwrite ? 'R' : 'W');

                ImGui::Separator();

                const char* mnemonic;
                char buf[DIS_DATAP_SIZE];
                if (datapath.jammed) {
                    mnemonic = "JAMMED";
                } else {
                    const auto wlen = dis_datapath(&snapshot, buf);
                    mnemonic = wlen < 0 ? dis_errstr(wlen) : buf;
                }
                ImGui::Text("Decode: %s", mnemonic);
                ImGui::Text("adl: %02X", datapath.addrlow_latch);
                ImGui::Text("adh: %02X", datapath.addrhigh_latch);
                ImGui::Text("adc: %02X", datapath.addrcarry_latch);
                ImGui::Text("t: %u", datapath.exec_cycle);

                ImGui::Separator();

                ImGui::BeginGroup();
                {
                    ImGui::Text("RDY:  %s", display_linestate(lines.ready));
                    ImGui::Text("SYNC: %s", display_linestate(lines.sync));
                    ImGui::Text("R/W:  %s",
                                display_linestate(lines.readwrite));
                }
                ImGui::EndGroup();

                ImGui::SameLine(0, 40);

                ImGui::BeginGroup();
                {
                    ImGui::Text("IRQ: %s%s", display_linestate(lines.irq),
                                display_signalstate(datapath.irq));
                    ImGui::Text("NMI: %s%s", display_linestate(lines.nmi),
                                display_signalstate(datapath.nmi));
                    ImGui::Text("RES: %s%s", display_linestate(lines.reset),
                                display_signalstate(datapath.res));
                }
                ImGui::EndGroup();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void aldo::RenderFrame::renderRam(const console_state& snapshot) const noexcept
{
    if (ImGui::Begin("RAM")) {
        static constexpr auto pageSize = 0x100, pageDim = 0x10,
                                cols = pageDim + 2, pageCount = 8;
        static constexpr auto tableConfig = ImGuiTableFlags_BordersOuter
                                            | ImGuiTableFlags_BordersV
                                            | ImGuiTableFlags_RowBg
                                            | ImGuiTableFlags_SizingFixedFit
                                            | ImGuiTableFlags_ScrollY;

        const ImVec2 tableSize{
            0, ImGui::GetTextLineHeightWithSpacing() * 2 * (pageDim + 1),
        };
        const auto spHighlightText =
            ImGui::ColorConvertU32ToFloat4(IM_COL32_BLACK);
        if (ImGui::BeginTable("ram", cols, tableConfig, tableSize)) {
            char col[3];
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Addr");
            for (auto i = 0; i < pageDim; ++i) {
                snprintf(col, sizeof col, " %01X", i);
                ImGui::TableSetupColumn(col);
            }
            ImGui::TableSetupColumn("ASCII");
            ImGui::TableHeadersRow();

            const auto sp = snapshot.cpu.stack_pointer;
            std::uint16_t addr = 0;
            for (std::size_t page = 0; page < pageCount; ++page) {
                for (std::size_t pageRow = 0; pageRow < pageDim; ++pageRow) {
                    if (page > 0 && pageRow == 0) {
                        ImGui::TableNextRow();
                        for (auto col = 0; col < cols; ++col) {
                            ImGui::TableSetColumnIndex(col);
                            ImGui::Dummy({0, ImGui::GetTextLineHeight()});
                        }
                    }
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%04X", addr);
                    const auto& lcl = std::locale::classic();
                    char ascii[pageDim + 1];
                    for (std::size_t ramCol = 0; ramCol < pageDim; ++ramCol) {
                        const auto ramIdx = (page * pageSize)
                                            + (pageRow * pageDim) + ramCol;
                        const auto val = snapshot.mem.ram[ramIdx];
                        ImGui::TableSetColumnIndex((int)ramCol + 1);
                        if (page == 1 && ramIdx % pageSize == sp) {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                                   IM_COL32(0xff, 0xfc, 0x53,
                                                            SDL_ALPHA_OPAQUE));
                            ImGui::TextColored(spHighlightText, "%02X", val);
                        } else {
                            ImGui::Text("%02X", val);
                        }
                        ascii[ramCol] = std::isprint((char)val, lcl)
                                        ? (char)val
                                        : '.';
                    }
                    ImGui::TableSetColumnIndex(cols - 1);
                    ascii[pageDim] = '\0';
                    ImGui::TextUnformatted(ascii);
                    addr += pageDim;
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

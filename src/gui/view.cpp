//
//  view.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/10/22.
//

#include "view.hpp"

#include "bytes.h"
#include "cart.h"
#include "ctrlsignal.h"
#include "dis.h"
#include "emulator.hpp"
#include "mediaruntime.hpp"
#include "viewstate.hpp"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <locale>
#include <string>
#include <string_view>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{

//
// Helpers
//

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

auto glyph_size() noexcept
{
    return ImGui::CalcTextSize("A");
}

template<std::derived_from<aldo::View>... Vs>
auto add_views(std::vector<std::unique_ptr<aldo::View>>& v, aldo::viewstate& s,
               const aldo::EmuController& c, const aldo::MediaRuntime& r)
{
    (v.push_back(std::make_unique<Vs>(s, c, r)), ...);
}

//
// Widgets
//

auto main_menu(aldo::viewstate& s, const aldo::MediaRuntime& r) noexcept
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(SDL_GetWindowTitle(r.window()))) {
            ImGui::MenuItem("About");
            ImGui::MenuItem("ImGui Demo", nullptr, &s.showDemo);
            if (ImGui::MenuItem("Quit", "Cmd+Q")) {
                s.events.emplace(aldo::Command::quit);
            };
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open", "Cmd+O")) {
                s.events.emplace(aldo::Command::openFile);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Bouncer", nullptr, &s.showBouncer);
            ImGui::MenuItem("CPU", nullptr, &s.showCpu);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

//
// Concrete Views
//

class HardwareTraits final : public aldo::View {
public:
    HardwareTraits(aldo::viewstate& s, const aldo::EmuController& c,
                   const aldo::MediaRuntime& r) noexcept
    : View{"Hardware Traits", s, c, r} {}

protected:
    void renderContents() const override
    {
        renderStats();
        ImGui::Separator();
        renderSpeedControls();
        ImGui::Separator();
        renderRunControls();
    }

private:
    void renderStats() const noexcept
    {
        static constexpr auto refreshIntervalMs = 250;
        static constinit double displayDtUpdate, refreshDt;

        const auto& cyclock = s.clock.cyclock;
        if ((refreshDt += cyclock.frametime_ms) >= refreshIntervalMs) {
            displayDtUpdate = s.clock.dtUpdateMs;
            refreshDt = 0;
        }
        ImGui::TextUnformatted("FPS: 60");
        ImGui::Text("Update dT: %.3f", displayDtUpdate);
        ImGui::Text("Frames: %" PRIu64, cyclock.frames);
        ImGui::Text("Runtime: %.3f", cyclock.runtime);
        ImGui::Text("Cycles: %" PRIu64, cyclock.total_cycles);
    }

    void renderSpeedControls() const noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cycles/Second");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::DragInt("##cyclesPerSecond", &s.clock.cyclock.cycles_per_sec,
                       1.0f, 1, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
    }

    void renderRunControls() const noexcept
    {
        const auto& snp = c.snapshot();
        auto halt = !snp.lines.ready;
        if (ImGui::Checkbox("HALT", &halt)) {
            s.events.emplace(aldo::Command::halt, halt);
        };

        ImGui::TextUnformatted("Mode");
        if (ImGui::RadioButton("Cycle", snp.mode == CSGM_CYCLE)
            && snp.mode != CSGM_CYCLE) {
            s.events.emplace(aldo::Command::mode, CSGM_CYCLE);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Step", snp.mode == CSGM_STEP)
            && snp.mode != CSGM_STEP) {
            s.events.emplace(aldo::Command::mode, CSGM_STEP);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Run", snp.mode == CSGM_RUN)
            && snp.mode != CSGM_RUN) {
            s.events.emplace(aldo::Command::mode, CSGM_RUN);
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
        auto
        irq = !snp.lines.irq, nmi = !snp.lines.nmi, res = !snp.lines.reset;
        if (ImGui::Checkbox("IRQ", &irq)) {
            s.events.emplace(aldo::Command::interrupt,
                             aldo::interrupt_event{CSGI_IRQ, irq});
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("NMI", &nmi)) {
            s.events.emplace(aldo::Command::interrupt,
                             aldo::interrupt_event{CSGI_NMI, nmi});
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("RES", &res)) {
            s.events.emplace(aldo::Command::interrupt,
                             aldo::interrupt_event{CSGI_RES, res});
        }
    }
};

class CartInfo final : public aldo::View {
public:
    CartInfo(aldo::viewstate& s, const aldo::EmuController& c,
             const aldo::MediaRuntime& r) noexcept
    : View{"Cart Info", s, c, r} {}

protected:
    void renderContents() const override
    {
        using namespace std::literals::string_view_literals;
        static constexpr auto label = "Name: "sv;

        const auto textSz = glyph_size(),
                    availSpace = ImGui::GetContentRegionAvail();
        const auto nameFit =
            static_cast<int>((availSpace.x / textSz.x) - label.length());
        const auto name = c.cartName();

        std::string_view trail;
        int nameLen;
        bool truncated;
        if (nameFit < static_cast<int>(name.length())) {
            trail = "..."sv;
            nameLen = std::max(0, nameFit - static_cast<int>(trail.length()));
            truncated = true;
        } else {
            trail = ""sv;
            nameLen = static_cast<int>(name.length());
            truncated = false;
        }
        ImGui::Text("%.*s%.*s%.*s", static_cast<int>(label.length()),
                    label.data(), nameLen, name.data(),
                    static_cast<int>(trail.length()), trail.data());
        if (truncated && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%.*s", static_cast<int>(name.length()),
                              name.data());
        }

        char cartFormat[CART_FMT_SIZE];
        const auto err = cart_format_extname(c.cartp(), cartFormat);
        ImGui::Text("Format: %s", err < 0 ? cart_errstr(err) : cartFormat);
    }
};

class PrgAtPc final : public aldo::View {
public:
    PrgAtPc(aldo::viewstate& s, const aldo::EmuController& c,
            const aldo::MediaRuntime& r) noexcept
    : View{"PRG @ PC", s, c, r} {}

protected:
    void renderContents() const override
    {
        renderPrg();
        renderVectors();
    }

private:
    void renderPrg() const noexcept
    {
        static constexpr auto instCount = 20;
        static constinit auto selected = -1;

        const auto& snp = c.snapshot();
        const auto& prgMem = snp.mem;
        auto addr = snp.datapath.current_instruction;
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
    }

    void renderVectors() const noexcept
    {
        if (ImGui::CollapsingHeader("Vectors",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& snp = c.snapshot();
            const auto& prgMem = snp.mem;

            auto lo = prgMem.vectors[0], hi = prgMem.vectors[1];
            ImGui::Text("%04X: %02X %02X     NMI $%04X", CPU_VECTOR_NMI, lo,
                        hi, bytowr(lo, hi));

            lo = prgMem.vectors[2];
            hi = prgMem.vectors[3];
            const auto& debugger = snp.debugger;
            const char* indicator;
            std::uint16_t resVector;
            if (debugger.resvector_override >= 0) {
                indicator = "!";
                resVector =
                    static_cast<std::uint16_t>(debugger.resvector_override);
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
};

class Bouncer final : public aldo::View {
public:
    Bouncer(aldo::viewstate& s, const aldo::EmuController& c,
            const aldo::MediaRuntime& r) noexcept
    : View{"Bouncer", s, c, r, &s.showBouncer} {}

protected:
    void renderContents() const override
    {
        const auto ren = r.renderer();
        const auto tex = r.bouncerTexture();
        SDL_SetRenderTarget(ren, tex);
        SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(ren, 30, 7, 50, 200);

        SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);

        const auto& bouncer = s.bouncer;
        const auto fulldim = bouncer.halfdim * 2;
        const SDL_Rect pos{
            bouncer.pos.x - bouncer.halfdim,
            bouncer.pos.y - bouncer.halfdim,
            fulldim,
            fulldim,
        };
        SDL_RenderFillRect(ren, &pos);
        SDL_SetRenderTarget(ren, nullptr);

        const ImVec2 sz{
            static_cast<float>(bouncer.bounds.x),
            static_cast<float>(bouncer.bounds.y),
        };
        ImGui::Image(tex, sz);
    }
};

class Cpu final : public aldo::View {
public:
    Cpu(aldo::viewstate& s, const aldo::EmuController& c,
        const aldo::MediaRuntime& r) noexcept
    : View{"CPU", s, c, r, &s.showCpu} {}

protected:
    void renderContents() const override
    {
        if (ImGui::BeginChild("CpuLeft", {200, 0})) {
            renderRegisters();
            renderFlags();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild("CpuRight")) {
            renderDatapath();
        }
        ImGui::EndChild();
    }

private:
    void renderRegisters() const noexcept
    {
        const auto& cpu = c.snapshot().cpu;
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
    }

    void renderFlags() const noexcept
    {
        static constexpr std::array flags{
            'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C',
        };
        static constexpr auto
            flagOn = IM_COL32(0xff, 0xfc, 0x53, SDL_ALPHA_OPAQUE),
            flagOff = IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE),
            textOn = IM_COL32_BLACK,
            textOff = IM_COL32_WHITE;

        if (ImGui::CollapsingHeader("Flags",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto textSz = glyph_size();
            const auto radius = (textSz.x + textSz.y) / 2.0f;
            const auto pos = ImGui::GetCursorScreenPos();
            ImVec2 center{pos.x + radius, pos.y + radius};
            const auto
                fontSz = ImGui::GetFontSize(),
                xOffset = fontSz / 4,
                yOffset = fontSz / 2;
            const auto drawList = ImGui::GetWindowDrawList();
            for (std::size_t i = 0; i < flags.size(); ++i) {
                ImU32 fillColor, textColor;
                if (c.snapshot().cpu.status & (1 << (flags.size() - 1 - i))) {
                    fillColor = flagOn;
                    textColor = textOn;
                } else {
                    fillColor = flagOff;
                    textColor = textOff;
                }
                drawList->AddCircleFilled(center, radius, fillColor);
                drawList->AddText({center.x - xOffset, center.y - yOffset},
                                  textColor, flags.data() + i,
                                  flags.data() + i + 1);
                center.x += 25;
            }
            ImGui::Dummy({0, radius * 2});
        }
    }

    void renderDatapath() const noexcept
    {
        if (ImGui::CollapsingHeader("Datapath",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& datapath = c.snapshot().datapath;
            const auto& lines = c.snapshot().lines;

            ImGui::Text("Address Bus: %04X", datapath.addressbus);
            const char* dataStr;
            std::array<char, 3> dataHex;
            if (datapath.busfault) {
                dataStr = "FLT";
            } else {
                std::snprintf(dataHex.data(), dataHex.size(), "%02X",
                              datapath.databus);
                dataStr = dataHex.data();
            }
            ImGui::Text("Data Bus: %s %c", dataStr,
                        lines.readwrite ? 'R' : 'W');

            ImGui::Separator();

            const char* mnemonic;
            char buf[DIS_DATAP_SIZE];
            if (datapath.jammed) {
                mnemonic = "JAMMED";
            } else {
                const auto err = dis_datapath(c.snapshotp(), buf);
                mnemonic = err < 0 ? dis_errstr(err) : buf;
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
};

class Ram final : public aldo::View {
public:
    Ram(aldo::viewstate& s, const aldo::EmuController& c,
        const aldo::MediaRuntime& r) noexcept
    : View{"RAM", s, c, r} {}

protected:
    void renderContents() const override
    {
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
            std::array<char, 3> col;
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Addr");
            for (auto i = 0; i < pageDim; ++i) {
                std::snprintf(col.data(), col.size(), " %01X", i);
                ImGui::TableSetupColumn(col.data());
            }
            ImGui::TableSetupColumn("ASCII");
            ImGui::TableHeadersRow();

            const auto& snp = c.snapshot();
            const auto sp = snp.cpu.stack_pointer;
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
                    std::string ascii(pageDim, '.');
                    for (std::size_t ramCol = 0; ramCol < pageDim; ++ramCol) {
                        const auto ramIdx = (page * pageSize)
                                            + (pageRow * pageDim) + ramCol;
                        const auto val = snp.mem.ram[ramIdx];
                        ImGui::TableSetColumnIndex(static_cast<int>(ramCol)
                                                   + 1);
                        if (page == 1 && ramIdx % pageSize == sp) {
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                                   IM_COL32(0xff, 0xfc, 0x53,
                                                            SDL_ALPHA_OPAQUE));
                            ImGui::TextColored(spHighlightText, "%02X", val);
                        } else {
                            ImGui::Text("%02X", val);
                        }
                        if (std::isprint(static_cast<char>(val), lcl)) {
                            ascii[ramCol] =
                                static_cast<std::string::value_type>(val);
                        }
                    }
                    ImGui::TableSetColumnIndex(cols - 1);
                    ImGui::TextUnformatted(ascii.c_str());
                    addr += pageDim;
                }
            }
            ImGui::EndTable();
        }
    }
};

}

//
// Public Interface
//

void aldo::View::View::render() const
{
    if (visible && !*visible) return;

    if (ImGui::Begin(title.c_str(), visible)) {
        renderContents();
    }
    ImGui::End();
}

aldo::Layout::Layout(aldo::viewstate& s, const aldo::EmuController& c,
                     const aldo::MediaRuntime& r)
: s{s}, r{r}
{
    add_views<
        HardwareTraits,
        CartInfo,
        PrgAtPc,
        Bouncer,
        Cpu,
        Ram
    >(views, s, c, r);
}

void aldo::Layout::render() const
{
    main_menu(s, r);
    for (const auto& v : views) {
        v->render();
    }
    if (s.showDemo) {
        ImGui::ShowDemoWindow();
    }
}

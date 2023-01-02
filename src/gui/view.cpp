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
#include "cycleclock.h"
#include "debug.h"
#include "dis.h"
#include "emulator.hpp"
#include "haltexpr.h"
#include "mediaruntime.hpp"
#include "style.hpp"
#include "viewstate.hpp"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <locale>
#include <string>
#include <string_view>
#include <type_traits>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace
{

//
// Helpers
//

constexpr auto NoSelection = -1;

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

constexpr auto boolstr(bool v) noexcept
{
    return v ? "yes" : "no";
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

auto main_menu(aldo::viewstate& s, const aldo::MediaRuntime& r)
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu(SDL_GetWindowTitle(r.window()))) {
            ImGui::MenuItem("About");
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
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &s.showDemo);
            if (ImGui::MenuItem("Aldo Studio")) {
                s.events.emplace(aldo::Command::launchStudio);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

auto input_address(std::uint16_t* addr) noexcept
{
    ImGui::SetNextItemWidth(glyph_size().x * 6);
    ImGui::PushID(addr);
    const auto result = ImGui::InputScalar("Address", ImGuiDataType_U16, addr,
                                           nullptr, nullptr, "%04X");
    ImGui::PopID();
    return result;
}

//
// Concrete Views
//

class Bouncer final : public aldo::View {
public:
    Bouncer(aldo::viewstate& s, const aldo::EmuController& c,
            const aldo::MediaRuntime& r) noexcept
    : View{"Bouncer", s, c, r} {}
    Bouncer(aldo::viewstate&, aldo::EmuController&&,
            const aldo::MediaRuntime&) = delete;
    Bouncer(aldo::viewstate&, const aldo::EmuController&,
            aldo::MediaRuntime&&) = delete;
    Bouncer(aldo::viewstate&, aldo::EmuController&&,
            aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
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

class CartInfo final : public aldo::View {
public:
    CartInfo(aldo::viewstate& s, const aldo::EmuController& c,
             const aldo::MediaRuntime& r) noexcept
    : View{"Cart Info", s, c, r} {}
    CartInfo(aldo::viewstate&, aldo::EmuController&&,
             const aldo::MediaRuntime&) = delete;
    CartInfo(aldo::viewstate&, const aldo::EmuController&,
             aldo::MediaRuntime&&) = delete;
    CartInfo(aldo::viewstate&, aldo::EmuController&&,
             aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
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

        const auto info = c.cartInfo();
        if (info) {
            ImGui::Text("Format: %s", cart_formatname(info->format));
            ImGui::Separator();
            switch (info->format) {
            case CRTF_INES:
                renderiNesInfo(info.value());
                break;
            default:
                renderRawInfo();
                break;
            }
        } else {
            ImGui::TextUnformatted("Format: None");
        }
    }

    void renderiNesInfo(const cartinfo& info) const noexcept
    {
        ImGui::Text("Mapper: %03u%s", info.ines_hdr.mapper_id,
                    info.ines_hdr.mapper_implemented
                        ? ""
                        : " (Not Implemented)");
        ImGui::TextUnformatted("Boards: <Board Names>");
        ImGui::Separator();
        ImGui::Text("PRG ROM: %u x 16KB", info.ines_hdr.prg_blocks);

        if (info.ines_hdr.wram) {
            ImGui::Text("WRAM: %u x 8KB",
                        std::max(static_cast<std::uint8_t>(1),
                                 info.ines_hdr.wram_blocks));
        } else {
            ImGui::TextUnformatted("WRAM: no");
        }

        if (info.ines_hdr.chr_blocks > 0) {
            ImGui::Text("CHR ROM: %u x 8KB", info.ines_hdr.chr_blocks);
            ImGui::TextUnformatted("CHR RAM: no");
        } else {
            ImGui::TextUnformatted("CHR ROM: no");
            ImGui::TextUnformatted("CHR RAM: 1 x 8KB");
        }

        ImGui::Text("NT-Mirroring: %s", cart_mirrorname(info.ines_hdr.mirror));
        ImGui::Text("Mapper-Ctrl: %s",
                    boolstr(info.ines_hdr.mapper_controlled));
        ImGui::Separator();
        ImGui::Text("Trainer: %s", boolstr(info.ines_hdr.trainer));
        ImGui::Text("Bus Conflicts: %s", boolstr(info.ines_hdr.bus_conflicts));
    }

    void renderRawInfo() const noexcept
    {
        // TODO: assume 32KB size for now
        ImGui::TextUnformatted("PRG ROM: 1 x 32KB");
    }
};

class Cpu final : public aldo::View {
public:
    Cpu(aldo::viewstate& s, const aldo::EmuController& c,
        const aldo::MediaRuntime& r) noexcept
    : View{"CPU", s, c, r} {}
    Cpu(aldo::viewstate&, aldo::EmuController&&,
        const aldo::MediaRuntime&) = delete;
    Cpu(aldo::viewstate&, const aldo::EmuController&,
        aldo::MediaRuntime&&) = delete;
    Cpu(aldo::viewstate&, aldo::EmuController&&,
        aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (ImGui::BeginChild("CpuLeft", {200, 0})) {
            if (ImGui::CollapsingHeader("Registers",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                renderRegisters();
            }
            if (ImGui::CollapsingHeader("Flags",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                renderFlags();
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild("CpuRight")) {
            if (ImGui::CollapsingHeader("Datapath",
                                        ImGuiTreeNodeFlags_DefaultOpen)) {
                renderDatapath();
            }
        }
        ImGui::EndChild();
    }

private:
    void renderRegisters() const noexcept
    {
        const auto& cpu = c.snapshot().cpu;
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

    void renderFlags() const noexcept
    {
        static constexpr std::array flags{
            'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C',
        };
        static constexpr auto textOn = IM_COL32_BLACK,
                                textOff = IM_COL32_WHITE;

        const auto textSz = glyph_size();
        const auto radius = (textSz.x + textSz.y) / 2.0f;
        const auto pos = ImGui::GetCursorScreenPos();
        ImVec2 center{pos.x + radius, pos.y + radius};
        const auto fontSz = ImGui::GetFontSize(),
                    xOffset = fontSz / 4,
                    yOffset = fontSz / 2;
        const auto drawList = ImGui::GetWindowDrawList();
        using flag_idx = decltype(flags)::size_type;
        for (flag_idx i = 0; i < flags.size(); ++i) {
            ImU32 fillColor, textColor;
            if (c.snapshot().cpu.status & (1 << (flags.size() - 1 - i))) {
                fillColor = aldo::colors::LedOn;
                textColor = textOn;
            } else {
                fillColor = aldo::colors::LedOff;
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

    void renderDatapath() const noexcept
    {
        const auto& datapath = c.snapshot().datapath;
        const auto& lines = c.snapshot().lines;

        ImGui::Text("Address Bus: %04X", datapath.addressbus);
        if (datapath.busfault) {
            ImGui::TextUnformatted("Data Bus: FLT");
        } else {
            std::array<char, 3> dataHex;
            std::snprintf(dataHex.data(), dataHex.size(), "%02X",
                          datapath.databus);
            ImGui::Text("Data Bus: %s", dataHex.data());
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(lines.readwrite ? "R" : "W");

        ImGui::Separator();

        if (datapath.jammed) {
            ImGui::TextUnformatted("Decode: JAMMED");
        } else {
            char buf[DIS_DATAP_SIZE];
            const auto err = dis_datapath(c.snapshotp(), buf);
            ImGui::Text("Decode: %s", err < 0 ? dis_errstr(err) : buf);
        }
        ImGui::Text("adl: %02X", datapath.addrlow_latch);
        ImGui::Text("adh: %02X", datapath.addrhigh_latch);
        ImGui::Text("adc: %02X", datapath.addrcarry_latch);
        ImGui::Text("t: %u", datapath.exec_cycle);

        ImGui::Separator();

        ImGui::BeginGroup();
        {
            ImGui::Text("RDY:  %s", display_linestate(lines.ready));
            ImGui::Text("SYNC: %s", display_linestate(lines.sync));
            ImGui::Text("R/W:  %s", display_linestate(lines.readwrite));
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
};

class Debugger final : public aldo::View {
public:
    Debugger(aldo::viewstate& s, const aldo::EmuController& c,
             const aldo::MediaRuntime& r) noexcept
    : View{"Debugger", s, c, r}
    {
        auto cond = static_cast<int>(HLT_NONE);
        for (auto& d : haltConditions) {
            const auto c = static_cast<haltcondition>(++cond);
            d = {c, haltcond_description(c)};
        }
        resetHaltExpression();
    }
    Debugger(aldo::viewstate&, aldo::EmuController&&,
             const aldo::MediaRuntime&) = delete;
    Debugger(aldo::viewstate&, const aldo::EmuController&,
             aldo::MediaRuntime&&) = delete;
    Debugger(aldo::viewstate&, aldo::EmuController&&,
             aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (ImGui::CollapsingHeader("Reset Vector",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            renderVectorOverride();
        }

        if (ImGui::CollapsingHeader("Breakpoints",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            renderBreakpoints();
        }
    }

private:
    using bpsize = std::remove_reference_t<decltype(c)>::bpsize;
    using bpindex = std::remove_reference_t<decltype(c)>::bpindex;

    void renderVectorOverride() noexcept
    {
        if (ImGui::Checkbox("Override", &resetOverride)) {
            if (resetOverride) {
                s.events.emplace(aldo::Command::overrideReset,
                                 static_cast<int>(resetAddr));
            } else {
                s.events.emplace(aldo::Command::overrideReset, NoResetVector);
            }
        }
        if (!resetOverride) {
            ImGui::BeginDisabled();
            // NOTE: +2 = start of reset vector
            resetAddr = batowr(c.snapshot().mem.vectors + 2);
        }
        if (input_address(&resetAddr)) {
            s.events.emplace(aldo::Command::overrideReset,
                             static_cast<int>(resetAddr));
        }
        if (!resetOverride) {
            ImGui::EndDisabled();
        }
    }

    void renderBreakpoints() noexcept
    {
        renderConditionCombo();
        ImGui::Separator();
        renderBreakpointAdd();
        ImGui::Separator();
        renderBreakpointList();
    }

    void renderConditionCombo() noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Halt on");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(glyph_size().x * 12);
        if (ImGui::BeginCombo("##haltconditions",
                              haltConditions[selectedCondition].second)) {
            for (haltindex i = 0; i < haltConditions.size(); ++i) {
                const auto current = i == selectedCondition;
                if (ImGui::Selectable(haltConditions[i].second, current)) {
                    selectedCondition = i;
                    resetHaltExpression();
                }
                if (current) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void renderBreakpointAdd() noexcept
    {
        switch (currentHaltExpression.cond) {
        case HLT_ADDR:
            input_address(&currentHaltExpression.address);
            break;
        case HLT_TIME:
            ImGui::SetNextItemWidth(glyph_size().x * 18);
            ImGui::InputFloat("Seconds", &currentHaltExpression.runtime);
            break;
        case HLT_CYCLES:
            ImGui::SetNextItemWidth(glyph_size().x * 18);
            ImGui::InputScalar("Count", ImGuiDataType_U64,
                               &currentHaltExpression.cycles);
            break;
        case HLT_JAM:
            ImGui::Dummy({0, ImGui::GetFrameHeight()});
            break;
        default:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Invalid halt condition selection: %zu",
                        selectedCondition);
            break;
        }
        if (ImGui::Button("Add")) {
            s.events.emplace(aldo::Command::breakpointAdd,
                             currentHaltExpression);
        }
    }

    void renderBreakpointList() noexcept
    {
        const ImVec2 dims{
            -FLT_MIN,
            8 * ImGui::GetTextLineHeightWithSpacing(),
        };
        const auto bpCount = c.breakpointCount();
        ImGui::Text("%zu breakpoint%s", bpCount, bpCount == 1 ? "" : "s");
        if (ImGui::BeginListBox("##breakpoints", dims)) {
            char fmt[HEXPR_FMT_SIZE];
            bpindex i = 0;
            for (auto bp = c.breakpointAt(i); bp; bp = c.breakpointAt(++i)) {
                const auto current = i == selectedBreakpoint;
                const int err = haltexpr_fmt(&bp->expr, fmt);
                if (!bp->enabled) {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,
                                        ImGui::GetStyle().DisabledAlpha);
                }
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(err < 0 ? haltexpr_errstr(err) : fmt,
                                      current)) {
                    selectedBreakpoint = i;
                }
                ImGui::PopID();
                if (!bp->enabled) {
                    ImGui::PopStyleVar();
                }
                if (current) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }
        renderListControls(bpCount);
    }

    void renderListControls(bpsize bpCount) noexcept
    {
        if (selectedBreakpoint == NoSelection) {
            ImGui::BeginDisabled();
        }
        auto bp = c.breakpointAt(selectedBreakpoint);
        if (ImGui::Button(!bp || bp->enabled ? "Disable" : "Enable ")) {
            s.events.emplace(aldo::Command::breakpointToggle,
                             selectedBreakpoint);
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,
                              aldo::colors::DestructiveButton);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              aldo::colors::DestructiveButtonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              aldo::colors::DestructiveButtonActive);
        auto resetSelection = false;
        if (ImGui::Button("Remove")) {
            s.events.emplace(aldo::Command::breakpointRemove,
                             selectedBreakpoint);
            resetSelection = true;
        }
        if (selectedBreakpoint == NoSelection) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (bpCount == 0) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Clear")) {
            s.events.emplace(aldo::Command::breakpointsClear);
            resetSelection = true;
        }
        if (bpCount == 0) {
            ImGui::EndDisabled();
        }
        ImGui::PopStyleColor(3);
        if (resetSelection) {
            selectedBreakpoint = NoSelection;
        }
    }

    void resetHaltExpression() noexcept
    {
        currentHaltExpression = {
            .cond = haltConditions[selectedCondition].first,
        };
    }

    bool resetOverride = false;
    std::uint16_t resetAddr = 0x0;

    // NOTE: does not include first enum value HLT_NONE
    std::array<std::pair<haltcondition, const char*>, HLT_CONDCOUNT - 1>
    haltConditions;
    using haltindex = decltype(haltConditions)::size_type;
    haltindex selectedCondition;
    haltexpr currentHaltExpression;
    bpindex selectedBreakpoint = NoSelection;
};

class HardwareTraits final : public aldo::View {
public:
    HardwareTraits(aldo::viewstate& s, const aldo::EmuController& c,
                   const aldo::MediaRuntime& r) noexcept
    : View{"Hardware Traits", s, c, r} {}
    HardwareTraits(aldo::viewstate&, aldo::EmuController&&,
                   const aldo::MediaRuntime&) = delete;
    HardwareTraits(aldo::viewstate&, const aldo::EmuController&,
                   aldo::MediaRuntime&&) = delete;
    HardwareTraits(aldo::viewstate&, aldo::EmuController&&,
                   aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        renderStats();
        ImGui::Separator();
        renderSpeedControls();
        ImGui::Separator();
        renderRunControls();
    }

private:
    void renderStats() noexcept
    {
        static constexpr auto refreshIntervalMs = 250;
        const auto& cyclock = s.clock.cyclock;
        if ((refreshDt += cyclock.frametime_ms) >= refreshIntervalMs) {
            dispDtInput = s.clock.dtInputMs;
            dispDtUpdate = s.clock.dtUpdateMs;
            dispDtRender = s.clock.dtRenderMs;
            refreshDt = 0;
        }
        ImGui::Text("Input dT: %.3f", dispDtInput);
        ImGui::Text("Update dT: %.3f", dispDtUpdate);
        ImGui::Text("Render dT: %.3f", dispDtRender);
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
                       1.0f, MinCps, MaxCps, "%d",
                       ImGuiSliderFlags_AlwaysClamp);
    }

    void renderRunControls() const
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

    double dispDtInput = 0, dispDtUpdate = 0, dispDtRender = 0, refreshDt = 0;
};

class PrgAtPc final : public aldo::View {
public:
    PrgAtPc(aldo::viewstate& s, const aldo::EmuController& c,
            const aldo::MediaRuntime& r) noexcept
    : View{"PRG @ PC", s, c, r} {}
    PrgAtPc(aldo::viewstate&, aldo::EmuController&&,
            const aldo::MediaRuntime&) = delete;
    PrgAtPc(aldo::viewstate&, const aldo::EmuController&,
            aldo::MediaRuntime&&) = delete;
    PrgAtPc(aldo::viewstate&, aldo::EmuController&&,
            aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        renderPrg();
        if (ImGui::CollapsingHeader("Vectors",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            renderVectors();
        }
    }

private:
    void renderPrg() noexcept
    {
        static constexpr auto instCount = 16;

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
        const auto& snp = c.snapshot();
        const auto& prgMem = snp.mem;

        auto lo = prgMem.vectors[0], hi = prgMem.vectors[1];
        ImGui::Text("%04X: %02X %02X     NMI $%04X", CPU_VECTOR_NMI, lo, hi,
                    bytowr(lo, hi));

        lo = prgMem.vectors[2];
        hi = prgMem.vectors[3];
        const auto& debugger = snp.debugger;
        const char* indicator;
        std::uint16_t resVector;
        if (debugger.resvector_override == NoResetVector) {
            indicator = "";
            resVector = bytowr(lo, hi);
        } else {
            indicator = "!";
            resVector =
                static_cast<std::uint16_t>(debugger.resvector_override);
        }
        ImGui::Text("%04X: %02X %02X     RES %s$%04X", CPU_VECTOR_RES, lo, hi,
                    indicator, resVector);

        lo = prgMem.vectors[4];
        hi = prgMem.vectors[5];
        ImGui::Text("%04X: %02X %02X     IRQ $%04X", CPU_VECTOR_IRQ, lo, hi,
                    bytowr(lo, hi));
    }

    int selected = NoSelection;
};

class Ram final : public aldo::View {
public:
    Ram(aldo::viewstate& s, const aldo::EmuController& c,
        const aldo::MediaRuntime& r) noexcept
    : View{"RAM", s, c, r} {}
    Ram(aldo::viewstate&, aldo::EmuController&&,
        const aldo::MediaRuntime&) = delete;
    Ram(aldo::viewstate&, const aldo::EmuController&,
        aldo::MediaRuntime&&) = delete;
    Ram(aldo::viewstate&, aldo::EmuController&&,
        aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
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

            const auto spHighlightText =
                ImGui::ColorConvertU32ToFloat4(IM_COL32_BLACK);
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
                                                   aldo::colors::LedOn);
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

void aldo::View::View::render()
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
        Bouncer,
        CartInfo,
        Cpu,
        Debugger,
        HardwareTraits,
        PrgAtPc,
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

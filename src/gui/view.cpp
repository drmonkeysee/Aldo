//
//  view.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/10/22.
//

#include "view.hpp"

#include "attr.hpp"
#include "bytes.h"
#include "cart.h"
#include "cpu.h"
#include "ctrlsignal.h"
#include "cycleclock.h"
#include "debug.hpp"
#include "dis.h"
#include "emu.hpp"
#include "emutypes.hpp"
#include "haltexpr.h"
#include "mediaruntime.hpp"
#include "snapshot.h"
#include "style.hpp"
#include "version.h"
#include "viewstate.hpp"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
#include <locale>
#include <set>
#include <string_view>
#include <type_traits>
#include <cinttypes>
#include <cstdio>

namespace
{

//
// Helpers
//

class ALDO_SIDEFX DisabledIf {
public:
    DisabledIf(bool condition) noexcept : condition{condition}
    {
        if (!condition) return;
        ImGui::BeginDisabled();
    };
    DisabledIf(const DisabledIf&) = delete;
    DisabledIf& operator=(const DisabledIf&) = delete;
    DisabledIf(DisabledIf&& that) noexcept : condition{that.condition}
    {
        that.condition = false;
    }
    // NOTE: this works because internally ImGui::Disabled is a stack, so the
    // order of Begin/End pairs doesn't matter as long as the stack is empty
    // at the end of an ImGui window scope.
    DisabledIf& operator=(DisabledIf&& that) noexcept
    {
        if (this == &that) return *this;

        swap(that);
        return *this;
    }
    ~DisabledIf()
    {
        if (!condition) return;
        ImGui::EndDisabled();
    }

    void swap(DisabledIf& that) noexcept
    {
        using std::swap;

        swap(condition, that.condition);
    }

private:
    bool condition;
};

[[maybe_unused]] void swap(DisabledIf& a, DisabledIf& b) noexcept
{
    a.swap(b);
}

template<typename T>
concept ScopedIDVal =
    std::convertible_to<T, void*> || std::convertible_to<T, int>;

class ALDO_SIDEFX ScopedID {
public:
    ScopedID(ScopedIDVal auto id) noexcept { ImGui::PushID(id); }
    ScopedID(const ScopedID&) = delete;
    ScopedID& operator=(const ScopedID&) = delete;
    ScopedID(ScopedID&&) = delete;
    ScopedID& operator=(ScopedID&&) = delete;
    ~ScopedID() { ImGui::PopID(); }
};

using scoped_style_val = std::pair<ImGuiStyleVar, float>;
using scoped_color_val = std::pair<ImGuiCol, ImU32>;
template<typename T>
concept ScopedVal =
    std::same_as<T, scoped_style_val> || std::same_as<T, scoped_color_val>;

template<ScopedVal V>
class ALDO_SIDEFX ScopedWidgetVars {
public:
    ScopedWidgetVars(V val, bool condition = true)
    : condition{condition}, count{1} { pushVars({val}); }
    ScopedWidgetVars(std::initializer_list<V> vals, bool condition = true)
    : condition{condition}, count{vals.size()} { pushVars(vals); }
    ScopedWidgetVars(const ScopedWidgetVars&) = delete;
    ScopedWidgetVars& operator=(const ScopedWidgetVars&) = delete;
    ScopedWidgetVars(ScopedWidgetVars&&) = delete;
    ScopedWidgetVars& operator=(ScopedWidgetVars&&) = delete;
    ~ScopedWidgetVars()
    {
        if (!condition) return;
        Policy::pop(static_cast<int>(count));
    }

private:
    void pushVars(std::initializer_list<V> vals) const
    {
        if (!condition) return;
        std::for_each(vals.begin(), vals.end(), Policy::push);
    }

    struct style_stack {
        static void push(const scoped_style_val& style) noexcept
        {
            ImGui::PushStyleVar(style.first, style.second);
        }
        static void pop(int count) noexcept
        {
            ImGui::PopStyleVar(count);
        }
    };
    struct color_stack {
        static void push(const scoped_color_val& color) noexcept
        {
            ImGui::PushStyleColor(color.first, color.second);
        }
        static void pop(int count) noexcept
        {
            ImGui::PopStyleColor(count);
        }
    };
    using Policy = std::conditional_t<std::same_as<V, scoped_style_val>,
                    style_stack,
                    color_stack>;

    bool condition;
    typename std::initializer_list<V>::size_type count;
};
using ScopedStyle = ScopedWidgetVars<scoped_style_val>;
using ScopedColor = ScopedWidgetVars<scoped_color_val>;

constexpr auto NoSelection = -1;

constexpr auto display_signalstate(csig_state s) noexcept
{
    switch (s) {
    case CSGS_PENDING:
        return "(P)";
    case CSGS_DETECTED:
        return "(D)";
    case CSGS_COMMITTED:
        return "(C)";
    case CSGS_SERVICED:
        return "(S)";
    default:
        return "(O)";
    }
}

constexpr auto boolstr(bool v) noexcept
{
    return v ? "yes" : "no";
}

constexpr auto operator+(ImVec2 v, float f) noexcept
{
    return ImVec2{v.x + f, v.y + f};
}

constexpr auto operator-(ImVec2 a, const ImVec2& b) noexcept
{
    return ImVec2{a.x - b.x, a.y - b.y};
}

auto keys_pressed(std::same_as<ImGuiKey> auto... keys) noexcept
{
    return (ImGui::IsKeyPressed(keys, false) || ...);
}

auto enter_pressed() noexcept
{
    return keys_pressed(ImGuiKey_Enter, ImGuiKey_KeypadEnter);
}

template<std::derived_from<aldo::View>... Vs>
auto add_views(aldo::Layout::view_list& vl, aldo::viewstate& vs,
               const aldo::Emulator& emu, const aldo::MediaRuntime& mr)
{
    (vl.push_back(std::make_unique<Vs>(vs, emu, mr)), ...);
}

//
// Widgets
//

auto widget_group(std::invocable auto f)
{
    ImGui::BeginGroup();
    f();
    ImGui::EndGroup();
}

auto about_submenu(aldo::viewstate& vs, const aldo::MediaRuntime& mr)
{
    if (ImGui::BeginMenu(SDL_GetWindowTitle(mr.window()))) {
        ImGui::MenuItem("About", nullptr, &vs.showAbout);
        if (ImGui::MenuItem("Quit", "Cmd+Q")) {
            vs.commands.emplace(aldo::Command::quit);
        };
        ImGui::EndMenu();
    }
}

auto file_menu(aldo::viewstate& vs, const aldo::Emulator& emu)
{
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open ROM...", "Cmd+O")) {
            vs.commands.emplace(aldo::Command::openROM);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Open Breakpoints...", "Cmd+B")) {
            vs.commands.emplace(aldo::Command::breakpointsOpen);
        }
        {
            const DisabledIf dif = !emu.debugger().isActive();
            if (ImGui::MenuItem("Export Breakpoints...", "Opt+Cmd+B")) {
                vs.commands.emplace(aldo::Command::breakpointsExport);
            }
        }
        ImGui::EndMenu();
    }
}

auto speed_menu_items(aldo::viewstate& vs) noexcept
{
    static constexpr auto multiplierLabel = " 10x";

    std::string incLabel = "Cycle Rate +", decLabel = "Cycle Rate -";
    const char* incKey, *decKey;
    int val;
    if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
        incLabel += multiplierLabel;
        decLabel += multiplierLabel;
        incKey = "+";
        decKey = "_";
        val = 10;
    } else {
        incKey = "=";
        decKey = "-";
        val = 1;
    }
    DisabledIf dif = vs.clock.cyclock.cycles_per_sec == MaxCps;
    if (ImGui::MenuItem(incLabel.c_str(), incKey)) {
        vs.clock.adjustCycleRate(val);
    }
    dif = vs.clock.cyclock.cycles_per_sec == MinCps;
    if (ImGui::MenuItem(decLabel.c_str(), decKey)) {
        vs.clock.adjustCycleRate(-val);
    }
}

auto mode_menu_item(aldo::viewstate& vs, const aldo::Emulator& emu)
{
    std::string label = "Switch Mode";
    const char* mnemonic;
    int modeAdjust;
    if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
        label += " (R)";
        mnemonic = "M";
        modeAdjust = -1;
    } else {
        mnemonic = "m";
        modeAdjust = 1;
    }
    if (ImGui::MenuItem(label.c_str(), mnemonic)) {
        const auto val = static_cast<csig_excmode>(emu.runMode() + modeAdjust);
        vs.commands.emplace(aldo::Command::mode, val);
    }
}

auto controls_menu(aldo::viewstate& vs, const aldo::Emulator& emu)
{
    using namespace std::literals::string_literals;

    if (ImGui::BeginMenu("Controls")) {
        speed_menu_items(vs);
        const auto& lines = emu.snapshot().lines;
        std::string haltLbl = (lines.ready ? "Halt"s : "Run"s) + " Emulator";
        if (ImGui::MenuItem(haltLbl.c_str(), "<Space>")) {
            vs.commands.emplace(aldo::Command::halt, lines.ready);
        }
        mode_menu_item(vs, emu);
        ImGui::Separator();
        // NOTE: interrupt signals are all low-active
        auto irq = !lines.irq, nmi = !lines.nmi, res = !lines.reset;
        if (ImGui::MenuItem("Send IRQ", "i", &irq)) {
            vs.addInterruptCommand(CSGI_IRQ, irq);
        }
        if (ImGui::MenuItem("Send NMI", "n", &nmi)) {
            vs.addInterruptCommand(CSGI_NMI, nmi);
        }
        if (ImGui::MenuItem("Send RES", "s", &res)) {
            vs.addInterruptCommand(CSGI_RES, res);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear RAM on Power-up", "Cmd+0", emu.zeroRam)) {
            vs.commands.emplace(aldo::Command::zeroRamOnPowerup, !emu.zeroRam);
        }
        ImGui::EndMenu();
    }
}

auto windows_menu(const aldo::Layout::view_list& vl)
{
    auto viewTransition = aldo::View::Transition::None;
    if (ImGui::BeginMenu("Windows")) {
        for (const auto& v : vl) {
            ImGui::MenuItem(v->windowTitle().c_str(), nullptr,
                            v->visibility());
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Open all")) {
            viewTransition = aldo::View::Transition::Open;
        }
        if (ImGui::MenuItem("Close all")) {
            viewTransition = aldo::View::Transition::Close;
        }
        if (ImGui::MenuItem("Expand all")) {
            viewTransition = aldo::View::Transition::Expand;
        }
        if (ImGui::MenuItem("Collapse all")) {
            viewTransition = aldo::View::Transition::Collapse;
        }
        ImGui::EndMenu();
    }
    if (viewTransition != aldo::View::Transition::None) {
        for (const auto& v : vl) {
            v->transition = viewTransition;
        }
    }
}

auto tools_menu(aldo::viewstate& vs)
{
    if (ImGui::BeginMenu("Tools")) {
        ImGui::MenuItem("ImGui Demo", "Cmd+D", &vs.showDemo);
        if (ImGui::MenuItem("Aldo Studio...")) {
            vs.commands.emplace(aldo::Command::launchStudio);
        }
        ImGui::EndMenu();
    }
}

auto main_menu(aldo::viewstate& vs, const aldo::Emulator& emu,
               const aldo::MediaRuntime& mr, const aldo::Layout::view_list& vl)
{
    if (ImGui::BeginMainMenuBar()) {
        about_submenu(vs, mr);
        file_menu(vs, emu);
        controls_menu(vs, emu);
        windows_menu(vl);
        tools_menu(vs);
        ImGui::EndMainMenuBar();
    }
}

auto input_address(aldo::et::word* addr) noexcept
{
    ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 6);
    const ScopedID id = addr;
    const auto result = ImGui::InputScalar("Address", ImGuiDataType_U16, addr,
                                           nullptr, nullptr, "%04X");
    return result;
}

auto about_overlay(aldo::viewstate& vs) noexcept
{
    if (!vs.showAbout) return;

    static constexpr auto flags = ImGuiWindowFlags_AlwaysAutoResize
                                    | ImGuiWindowFlags_NoDecoration
                                    | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoNav
                                    | ImGuiWindowFlags_NoSavedSettings;

    const auto workArea = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(workArea + 25.0f);
    ImGui::SetNextWindowBgAlpha(0.75f);

    if (ImGui::Begin("About Aldo", nullptr, flags)) {
        ImGui::LogToClipboard();
        ImGui::Text("Aldo %s", AldoVersion);
    #ifdef __VERSION__
        ImGui::TextUnformatted(__VERSION__);
    #endif
        SDL_version v;
        SDL_VERSION(&v);
        ImGui::Text("SDL %u.%u.%u", v.major, v.minor, v.patch);
        ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
        ImGui::LogFinish();

        ImGui::Separator();
        ImGui::TextUnformatted("Text has been copied to clipboard");

        if (keys_pressed(ImGuiKey_Escape)
            || (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && !ImGui::IsWindowHovered())) {
            vs.showAbout = false;
        }
    }
    ImGui::End();
}

//
// Concrete Views
//

class BouncerView final : public aldo::View {
public:
    BouncerView(aldo::viewstate& vs, const aldo::Emulator& emu,
                const aldo::MediaRuntime& mr) noexcept
    : View{"Bouncer", vs, emu, mr} {}
    BouncerView(aldo::viewstate&, aldo::Emulator&&,
                const aldo::MediaRuntime&) = delete;
    BouncerView(aldo::viewstate&, const aldo::Emulator&,
                aldo::MediaRuntime&&) = delete;
    BouncerView(aldo::viewstate&, aldo::Emulator&&,
                aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        const auto ren = mr.renderer();
        const auto tex = mr.bouncerTexture();
        SDL_SetRenderTarget(ren, tex);
        SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(ren, 30, 7, 50, 200);

        SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);

        const auto& bouncer = vs.bouncer;
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

class CartInfoView final : public aldo::View {
public:
    CartInfoView(aldo::viewstate& vs, const aldo::Emulator& emu,
                 const aldo::MediaRuntime& mr) noexcept
    : View{"Cart Info", vs, emu, mr} {}
    CartInfoView(aldo::viewstate&, aldo::Emulator&&,
                 const aldo::MediaRuntime&) = delete;
    CartInfoView(aldo::viewstate&, const aldo::Emulator&,
                 aldo::MediaRuntime&&) = delete;
    CartInfoView(aldo::viewstate&, aldo::Emulator&&,
                 aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        using namespace std::literals::string_view_literals;
        static constexpr auto label = "Name: "sv;

        const auto textSz = aldo::style::glyph_size(),
                    availSpace = ImGui::GetContentRegionAvail();
        const auto nameFit =
            static_cast<int>((availSpace.x / textSz.x) - label.length());
        const auto name = emu.displayCartName();

        std::string_view trail;
        int nameLen;
        bool truncated;
        if (nameFit < std::ssize(name)) {
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

        const auto info = emu.cartInfo();
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

private:
    static void renderiNesInfo(const cartinfo& info) noexcept
    {
        ImGui::Text("Mapper: %03u", info.ines_hdr.mapper_id);
        if (!info.ines_hdr.mapper_implemented) {
            ImGui::SameLine();
            ImGui::TextUnformatted("(Not Implemented)");
        }
        ImGui::TextUnformatted("Boards: <Board Names>");
        ImGui::Separator();
        ImGui::Text("PRG ROM: %u x 16KB", info.ines_hdr.prg_blocks);

        if (info.ines_hdr.wram) {
            ImGui::Text("WRAM: %u x 8KB",
                        std::max(static_cast<aldo::et::byte>(1),
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

    static void renderRawInfo() noexcept
    {
        // TODO: assume 32KB size for now
        ImGui::TextUnformatted("PRG ROM: 1 x 32KB");
    }
};

class CpuView final : public aldo::View {
public:
    CpuView(aldo::viewstate& vs, const aldo::Emulator& emu,
            const aldo::MediaRuntime& mr) noexcept
    : View{"CPU", vs, emu, mr} {}
    CpuView(aldo::viewstate&, aldo::Emulator&&,
            const aldo::MediaRuntime&) = delete;
    CpuView(aldo::viewstate&, const aldo::Emulator&,
            aldo::MediaRuntime&&) = delete;
    CpuView(aldo::viewstate&, aldo::Emulator&&, aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (ImGui::CollapsingHeader("Registers",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            renderRegisters();
        }
        if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderFlags();
        }
    }

private:
    void renderRegisters() const noexcept
    {
        const auto& cpu = emu.snapshot().cpu;
        widget_group([&cpu] {
            ImGui::Text("A: %02X", cpu.accumulator);
            ImGui::Text("X: %02X", cpu.xindex);
            ImGui::Text("Y: %02X", cpu.yindex);
        });
        ImGui::SameLine(0, 90);
        widget_group([&cpu] {
            ImGui::Text("PC: %04X", cpu.program_counter);
            ImGui::Text(" S: %02X", cpu.stack_pointer);
            ImGui::Text(" P: %02X", cpu.status);
        });
    }

    void renderFlags() const
    {
        static constexpr std::array flags{
            'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C',
        };
        static constexpr auto
            textOn = IM_COL32_BLACK,
            textOff = IM_COL32_WHITE;

        const auto textSz = aldo::style::glyph_size();
        const auto radius = (textSz.x + textSz.y) / 2.0f;
        const auto pos = ImGui::GetCursorScreenPos();
        ImVec2 center = pos + radius;

        const auto fontSz = ImGui::GetFontSize();
        const ImVec2 offset{fontSz / 4, fontSz / 2};
        const auto drawList = ImGui::GetWindowDrawList();

        for (auto it = flags.cbegin(); it != flags.cend(); ++it) {
            const auto bitpos = std::distance(it, flags.cend()) - 1;
            ImU32 fillColor, textColor;
            if (emu.snapshot().cpu.status & (1 << bitpos)) {
                fillColor = aldo::colors::LedOn;
                textColor = textOn;
            } else {
                fillColor = aldo::colors::LedOff;
                textColor = textOff;
            }
            drawList->AddCircleFilled(center, radius, fillColor);
            drawList->AddText(center - offset, textColor, it, it + 1);
            center.x += radius * 2.5f;
        }
        ImGui::Dummy({0, radius * 2});
    }
};

class DatapathView final : public aldo::View {
public:
    DatapathView(aldo::viewstate& vs, const aldo::Emulator& emu,
                 const aldo::MediaRuntime& mr) noexcept
    : View{"Datapath", vs, emu, mr} {}
    DatapathView(aldo::viewstate&, aldo::Emulator&&,
                 const aldo::MediaRuntime&) = delete;
    DatapathView(aldo::viewstate&, const aldo::Emulator&,
                 aldo::MediaRuntime&&) = delete;
    DatapathView(aldo::viewstate&, aldo::Emulator&&,
                 aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        const auto glyphW = aldo::style::glyph_size().x,
                    lineSpacer = glyphW * 6;
        renderControlLines(lineSpacer, glyphW);
        ImGui::Separator();
        renderBusLines();
        ImGui::Separator();
        renderInstructionDecode();
        ImGui::Separator();
        renderInterruptLines(lineSpacer);
    }

private:
    void renderControlLines(float spacer, float adjustW) const noexcept
    {
        const auto& lines = emu.snapshot().lines;
        renderControlLine("RDY", lines.ready);
        ImGui::SameLine(0, spacer);
        renderControlLine("SYNC", lines.sync);
        // NOTE: adjust spacer width for SYNC's extra letter
        ImGui::SameLine(0, spacer - adjustW);
        renderRWLine(lines.readwrite, adjustW);
    }

    void renderBusLines() const noexcept
    {
        const auto& datapath = emu.snapshot().datapath;
        {
            const ScopedColor color{{ImGuiCol_Text, aldo::colors::LineOut}};
            ImGui::Text("Addr: %04X", datapath.addressbus);
        };
        ImGui::SameLine(0, 20);
        if (datapath.busfault) {
            const ScopedColor color{
                {ImGuiCol_Text, aldo::colors::DestructiveHover},
            };
            ImGui::TextUnformatted("Data: FLT");
        } else {
            std::array<char, 3> dataHex;
            std::snprintf(dataHex.data(), dataHex.size(), "%02X",
                          datapath.databus);
            const ScopedColor color{{
                ImGuiCol_Text,
                emu.snapshot().lines.readwrite
                    ? aldo::colors::LineIn
                    : aldo::colors::LineOut,
            }};
            ImGui::Text("Data: %s", dataHex.data());
        }
    }

    void renderInstructionDecode() const noexcept
    {
        const auto& datapath = emu.snapshot().datapath;
        if (datapath.jammed) {
            ScopedColor color{{ImGuiCol_Text, aldo::colors::DestructiveHover}};
            ImGui::TextUnformatted("Decode: JAMMED");
        } else {
            std::array<aldo::et::tchar, DIS_DATAP_SIZE> buf;
            const auto err = dis_datapath(emu.snapshotp(), buf.data());
            ImGui::Text("Decode: %s", err < 0 ? dis_errstr(err) : buf.data());
        }
        ImGui::Text("adl: %02X", datapath.addrlow_latch);
        ImGui::Text("adh: %02X", datapath.addrhigh_latch);
        ImGui::Text("adc: %02X", datapath.addrcarry_latch);
        renderCycleIndicator(emu.snapshot().datapath.exec_cycle);
    }

    void renderInterruptLines(float spacer) const noexcept
    {
        ImGui::Spacing();

        const auto& lines = emu.snapshot().lines;
        renderInterruptLine("IRQ", lines.irq);
        ImGui::SameLine(0, spacer);
        renderInterruptLine("NMI", lines.nmi);
        ImGui::SameLine(0, spacer);
        renderInterruptLine("RES", lines.reset);

        const auto& datapath = emu.snapshot().datapath;
        ImGui::TextUnformatted(display_signalstate(datapath.irq));
        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted(display_signalstate(datapath.nmi));
        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted(display_signalstate(datapath.res));
    }

    static void renderCycleIndicator(aldo::et::byte cycle) noexcept
    {
        static constexpr auto radius = 5;

        ImGui::TextUnformatted("t:");
        ImGui::SameLine();
        const auto pos = ImGui::GetCursorScreenPos();
        ImVec2 center{pos.x, pos.y + (ImGui::GetTextLineHeight() / 2) + 1};
        const auto drawList = ImGui::GetWindowDrawList();
        for (auto i = 0; i < MaxTCycle; ++i) {
            drawList->AddCircleFilled(center, radius,
                                      i == cycle
                                        ? aldo::colors::LedOn
                                        : aldo::colors::LedOff);
            center.x += radius * 3;
        }
        ImGui::Spacing();
    }

    static void renderControlLine(const char* label, bool active) noexcept
    {
        const DisabledIf dif = !active;
        ImGui::TextUnformatted(label);
    }

    static void renderRWLine(bool active, float adjustW) noexcept
    {
        const DisabledIf dif = !active;
        ImGui::TextUnformatted("R/W");
        const ImVec2
            end{ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y},
            start{end.x - adjustW, end.y};
        const auto drawList = ImGui::GetWindowDrawList();
        drawList->AddLine(start, end, aldo::colors::white(active));
    }

    static void renderInterruptLine(const char* label, bool active) noexcept
    {
        const DisabledIf dif = !active;
        ImGui::TextUnformatted(label);
        const auto start = ImGui::GetItemRectMin();
        const ImVec2 end{
            start.x + ImGui::GetItemRectSize().x, start.y,
        };
        const auto drawlist = ImGui::GetWindowDrawList();
        drawlist->AddLine(start, end, aldo::colors::white(active));
    }
};

class DebuggerView final : public aldo::View {
public:
    DebuggerView(aldo::viewstate& vs, const aldo::Emulator& emu,
                 const aldo::MediaRuntime& mr)
    : View{"Debugger", vs, emu, mr}
    {
        using halt_val = halt_array::value_type;
        using halt_integral = std::underlying_type_t<halt_val::first_type>;

        // TODO: drop parens and use ranges in C++23?
        std::generate(haltConditions.begin(), haltConditions.end(),
            [h = static_cast<halt_integral>(HLT_NONE)]() mutable -> halt_val {
                const auto cond = static_cast<halt_val::first_type>(++h);
                return {cond, haltcond_description(cond)};
            });
        resetHaltExpression(haltConditions.cbegin());
    }
    DebuggerView(aldo::viewstate&, aldo::Emulator&&,
                 const aldo::MediaRuntime&) = delete;
    DebuggerView(aldo::viewstate&, const aldo::Emulator&,
                 aldo::MediaRuntime&&) = delete;
    DebuggerView(aldo::viewstate&, aldo::Emulator&&,
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
    // NOTE: does not include first enum value HLT_NONE
    using halt_array =
        std::array<std::pair<haltcondition, aldo::et::str>, HLT_CONDCOUNT - 1>;
    using halt_it = halt_array::const_iterator;
    using bp_sz = aldo::Debugger::BpView::size_type;
    using bp_diff = aldo::Debugger::BreakpointIterator::difference_type;

    void renderVectorOverride() noexcept
    {
        syncWithOverrideState();

        if (ImGui::Checkbox("Override", &resetOverride)) {
            if (resetOverride) {
                vs.commands.emplace(aldo::Command::resetVectorOverride,
                                    static_cast<int>(resetAddr));
            } else {
                vs.commands.emplace(aldo::Command::resetVectorClear);
            }
        }
        const DisabledIf dif = [this] {
            if (this->resetOverride) return false;
            // NOTE: +2 = start of reset vector
            this->resetAddr = batowr(this->emu.snapshot().mem.vectors + 2);
            return true;
        }();
        if (input_address(&resetAddr)) {
            vs.commands.emplace(aldo::Command::resetVectorOverride,
                                static_cast<int>(resetAddr));
        }
    }

    void renderBreakpoints()
    {
        const auto setFocus = renderConditionCombo();
        ImGui::Separator();
        renderBreakpointAdd(setFocus);
        ImGui::Separator();
        renderBreakpointList();
        detectedHalt = emu.haltedByDebugger();
    }

    bool renderConditionCombo() noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Halt on");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 12);
        auto setFocus = false;
        if (ImGui::BeginCombo("##haltconditions", selectedCondition->second)) {
            for (auto it = haltConditions.cbegin();
                 it != haltConditions.cend();
                 ++it) {
                const auto current = it == selectedCondition;
                if (ImGui::Selectable(it->second, current)) {
                    setFocus = true;
                    if (!current) {
                        resetHaltExpression(it);
                    }
                }
                if (current) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        return setFocus;
    }

    void renderBreakpointAdd(bool setFocus) noexcept
    {
        switch (currentHaltExpression.cond) {
        case HLT_ADDR:
            input_address(&currentHaltExpression.address);
            break;
        case HLT_TIME:
            ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 18);
            ImGui::InputFloat("Seconds", &currentHaltExpression.runtime);
            break;
        case HLT_CYCLES:
            ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 18);
            ImGui::InputScalar("Count", ImGuiDataType_U64,
                               &currentHaltExpression.cycles);
            break;
        case HLT_JAM:
            ImGui::Dummy({0, ImGui::GetFrameHeight()});
            break;
        default:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Invalid halt condition selection: %d",
                        selectedCondition->first);
            break;
        }
        if (setFocus) {
            ImGui::SetKeyboardFocusHere(-1);
        }
        const auto submitted = ImGui::IsItemDeactivated() && enter_pressed();
        if (ImGui::Button("Add") || submitted) {
            vs.commands.emplace(aldo::Command::breakpointAdd,
                                currentHaltExpression);
        }
    }

    void renderBreakpointList()
    {
        const ImVec2 dims{-FLT_MIN, 8 * ImGui::GetTextLineHeightWithSpacing()};
        const auto bpView = emu.debugger().breakpoints();
        const auto bpCount = bpView.size();
        ImGui::Text("%zu breakpoint%s", bpCount, bpCount == 1 ? "" : "s");
        if (ImGui::BeginListBox("##breakpoints", dims)) {
            bp_diff i = 0;
            for (const auto& bp : bpView) {
                renderBreakpoint(i++, bp);
            }
            ImGui::EndListBox();
        }
        renderListControls(bpCount);
    }

    void renderBreakpoint(bp_diff idx, const breakpoint& bp)
    {
        const auto bpBreak = idx == emu.snapshot().debugger.halted;
        if (bpBreak && !detectedHalt) {
            selectBreakpoint(idx);
        }
        std::array<aldo::et::tchar, HEXPR_FMT_SIZE> fmt;
        const auto err = haltexpr_desc(&bp.expr, fmt.data());
        const ScopedStyle style{
            {ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha},
            !bp.enabled,
        };
        const ScopedColor color{
            {ImGuiCol_Text, aldo::colors::Attention}, bpBreak,
        };
        const ScopedID id = static_cast<int>(idx);
        if (ImGui::Selectable(err < 0 ? haltexpr_errstr(err) : fmt.data(),
                              bpSelections.contains(idx))) {
            if (ImGui::GetIO().KeyMods) {
                bpSelections.insert(idx);
            } else {
                selectBreakpoint(idx);
            }
        }
    }

    void renderListControls(bp_sz bpCount)
    {
        DisabledIf dif = bpSelections.empty();
        const auto& dbg = emu.debugger();
        const auto bp = dbg.breakpoints().at(firstBpSelection());
        if (ImGui::Button(!bp || bp->enabled ? "Disable" : "Enable ")) {
            for (const auto idx : bpSelections) {
                const auto cmd = bp->enabled
                                    ? aldo::Command::breakpointDisable
                                    : aldo::Command::breakpointEnable;
                vs.commands.emplace(cmd, idx);
            }
        }
        ImGui::SameLine();
        const ScopedColor colors = {
            {ImGuiCol_Button, aldo::colors::Destructive},
            {ImGuiCol_ButtonHovered, aldo::colors::DestructiveHover},
            {ImGuiCol_ButtonActive, aldo::colors::DestructiveActive},
        };
        auto resetSelection = false;
        if (ImGui::Button("Remove")) {
            // NOTE: queue remove commands in reverse order to avoid
            // invalidating bp indices during removal.
            // TODO: when apple clang gets ranges use those
            for (auto it = bpSelections.crbegin();
                 it != bpSelections.crend();
                 ++it) {
                vs.commands.emplace(aldo::Command::breakpointRemove, *it);
            }
            resetSelection = true;
        }
        ImGui::SameLine();
        dif = bpCount == 0;
        if (ImGui::Button("Clear")) {
            vs.commands.emplace(aldo::Command::breakpointsClear);
            resetSelection = true;
        }
        if (resetSelection) {
            bpSelections.clear();
        }
    }

    void resetHaltExpression(halt_it selection) noexcept
    {
        selectedCondition = selection;
        currentHaltExpression = {.cond = selectedCondition->first};
    }

    void syncWithOverrideState() noexcept
    {
        // NOTE: if view flag doesn't match debugger state then debugger
        // state was changed under the hood (e.g. by loading a brk file).
        // NOTE: if another case like this comes up it may be worth coming up
        // with a model -> view notification system.
        const auto& dbg = emu.debugger();
        if ((resetOverride = dbg.isVectorOverridden())) {
            resetAddr = static_cast<aldo::et::word>(dbg.vectorOverride());
        }
    }

    void selectBreakpoint(bp_diff idx)
    {
        bpSelections.clear();
        bpSelections.insert(idx);
    }

    bp_diff firstBpSelection() const noexcept
    {
        return bpSelections.empty()
                ? NoSelection
                : *bpSelections.cbegin();
    }

    bool detectedHalt = false, resetOverride = false;
    aldo::et::word resetAddr = 0x0;
    halt_array haltConditions;
    // NOTE: storing iterator is safe as haltConditions
    // is immutable for the life of this instance.
    halt_it selectedCondition;
    haltexpr currentHaltExpression;
    std::set<bp_diff> bpSelections;
};

class HardwareTraitsView final : public aldo::View {
public:
    HardwareTraitsView(aldo::viewstate& vs, const aldo::Emulator& emu,
                       const aldo::MediaRuntime& mr) noexcept
    : View{"Hardware Traits", vs, emu, mr} {}
    HardwareTraitsView(aldo::viewstate&, aldo::Emulator&&,
                       const aldo::MediaRuntime&) = delete;
    HardwareTraitsView(aldo::viewstate&, const aldo::Emulator&,
                       aldo::MediaRuntime&&) = delete;
    HardwareTraitsView(aldo::viewstate&, aldo::Emulator&&,
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
        const auto& cyclock = vs.clock.cyclock;
        if ((refreshDt += cyclock.frametime_ms) >= refreshIntervalMs) {
            dispDtInput = vs.clock.dtInputMs;
            dispDtUpdate = vs.clock.dtUpdateMs;
            dispDtRender = vs.clock.dtRenderMs;
            dispDtTotal = dispDtInput + dispDtUpdate + dispDtRender;
            refreshDt = 0;
        }
        ImGui::Text("Input dT: %.3f", dispDtInput);
        ImGui::Text("Update dT: %.3f", dispDtUpdate);
        ImGui::Text("Render dT: %.3f", dispDtRender);
        ImGui::Text("Total dT: %.3f", dispDtTotal);
        ImGui::Text("Frames: %" PRIu64, cyclock.frames);
        ImGui::Text("Runtime: %.3f", cyclock.runtime);
        ImGui::Text("Cycles: %" PRIu64, cyclock.total_cycles);
        ImGui::Text("BCD Support: %s", boolstr(emu.bcdSupport()));
    }

    void renderSpeedControls() const noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cycles/Second");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::DragInt("##cyclesPerSecond", &vs.clock.cyclock.cycles_per_sec,
                       1.0f, MinCps, MaxCps, "%d",
                       ImGuiSliderFlags_AlwaysClamp);
    }

    void renderRunControls() const
    {
        const auto& lines = emu.snapshot().lines;
        auto halt = !lines.ready;
        if (ImGui::Checkbox("HALT", &halt)) {
            vs.commands.emplace(aldo::Command::halt, halt);
        };

        const auto mode = emu.runMode();
        ImGui::TextUnformatted("Mode");
        if (ImGui::RadioButton("Cycle", mode == CSGM_CYCLE)
            && mode != CSGM_CYCLE) {
            vs.commands.emplace(aldo::Command::mode, CSGM_CYCLE);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Step", mode == CSGM_STEP)
            && mode != CSGM_STEP) {
            vs.commands.emplace(aldo::Command::mode, CSGM_STEP);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Run", mode == CSGM_RUN) && mode != CSGM_RUN) {
            vs.commands.emplace(aldo::Command::mode, CSGM_RUN);
        }

        ImGui::TextUnformatted("Signal");
        // NOTE: interrupt signals are all low-active
        auto irq = !lines.irq, nmi = !lines.nmi, res = !lines.reset;
        if (ImGui::Checkbox("IRQ", &irq)) {
            vs.addInterruptCommand(CSGI_IRQ, irq);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("NMI", &nmi)) {
            vs.addInterruptCommand(CSGI_NMI, nmi);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("RES", &res)) {
            vs.addInterruptCommand(CSGI_RES, res);
        }
    }

    double dispDtInput = 0, dispDtUpdate = 0, dispDtRender = 0,
            dispDtTotal = 0, refreshDt = 0;
};

class PrgAtPcView final : public aldo::View {
public:
    PrgAtPcView(aldo::viewstate& vs, const aldo::Emulator& emu,
                const aldo::MediaRuntime& mr) noexcept
    : View{"PRG @ PC", vs, emu, mr} {}
    PrgAtPcView(aldo::viewstate&, aldo::Emulator&&,
                const aldo::MediaRuntime&) = delete;
    PrgAtPcView(aldo::viewstate&, const aldo::Emulator&,
                aldo::MediaRuntime&&) = delete;
    PrgAtPcView(aldo::viewstate&, aldo::Emulator&&,
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

        const auto& snp = emu.snapshot();
        const auto& prgMem = snp.mem;
        auto addr = snp.datapath.current_instruction;
        dis_instruction inst{};
        std::array<aldo::et::tchar, DIS_INST_SIZE> disasm;
        for (int i = 0; i < instCount; ++i) {
            auto result = dis_parsemem_inst(prgMem.prglength,
                                            prgMem.currprg,
                                            inst.offset + inst.bv.size,
                                            &inst);
            if (result > 0) {
                result = dis_inst(addr, &inst, disasm.data());
                if (result > 0) {
                    if (ImGui::Selectable(disasm.data(), selected == i)) {
                        selected = i;
                    } else if (ImGui::BeginPopupContextItem()) {
                        selected = i;
                        if (ImGui::Selectable("Add breakpoint...")) {
                            const auto expr = haltexpr{
                                .address = addr, .cond = HLT_ADDR,
                            };
                            vs.commands.emplace(aldo::Command::breakpointAdd,
                                                expr);
                        }
                        ImGui::EndPopup();
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
        const auto& snp = emu.snapshot();
        const auto& prgMem = snp.mem;

        auto lo = prgMem.vectors[0], hi = prgMem.vectors[1];
        ImGui::Text("%04X: %02X %02X     NMI $%04X", CPU_VECTOR_NMI, lo, hi,
                    bytowr(lo, hi));

        lo = prgMem.vectors[2];
        hi = prgMem.vectors[3];
        const char* indicator;
        aldo::et::word resVector;
        const auto& dbg = emu.debugger();
        if (dbg.isVectorOverridden()) {
            indicator = HEXPR_RES_IND;
            resVector = static_cast<aldo::et::word>(dbg.vectorOverride());
        } else {
            indicator = "";
            resVector = bytowr(lo, hi);
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

class RamView final : public aldo::View {
public:
    RamView(aldo::viewstate& vs, const aldo::Emulator& emu,
            const aldo::MediaRuntime& mr) noexcept
    : View{"RAM", vs, emu, mr} {}
    RamView(aldo::viewstate&, aldo::Emulator&&,
            const aldo::MediaRuntime&) = delete;
    RamView(aldo::viewstate&, const aldo::Emulator&,
            aldo::MediaRuntime&&) = delete;
    RamView(aldo::viewstate&, aldo::Emulator&&, aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        static constexpr auto tableConfig = ImGuiTableFlags_BordersOuter
                                            | ImGuiTableFlags_BordersV
                                            | ImGuiTableFlags_RowBg
                                            | ImGuiTableFlags_SizingFixedFit
                                            | ImGuiTableFlags_ScrollY;

        const auto pageCount = static_cast<int>(emu.ramSize()) / PageSize,
                    rowCount = pageCount * PageDim;
        const ImVec2 tableSize{
            0, ImGui::GetTextLineHeightWithSpacing() * 2 * (PageDim + 1),
        };
        if (ImGui::BeginTable("ram", Cols, tableConfig, tableSize)) {
            renderHeader();
            ImGuiListClipper clip;
            // NOTE: account for up to 2 page-break rows per screen
            clip.Begin(rowCount + 2);
            RamTable tbl{emu, rowCount};
            while(clip.Step()) {
                tbl.renderRows(clip.DisplayStart, clip.DisplayEnd);
            }
            ImGui::EndTable();
        }
    }

private:
    static void renderHeader() noexcept
    {
        std::array<char, 3> col;
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Addr");
        for (auto i = 0; i < PageDim; ++i) {
            std::snprintf(col.data(), col.size(), " %01X", i);
            ImGui::TableSetupColumn(col.data());
        }
        ImGui::TableSetupColumn("ASCII");
        ImGui::TableHeadersRow();
    }

    class RamTable {
    public:
        RamTable(const aldo::Emulator& emu, int rowCount)
        : ascii(static_cast<ascii_sz>(PageDim), Placeholder),
            totalRows{rowCount}, ram{emu.snapshot().mem.ram},
            sp{emu.snapshot().cpu.stack_pointer} {}
        RamTable(aldo::Emulator&& emu, int rowCount) = delete;

        void renderRows(int start, int end)
        {
            for (auto row = start; row < end; ++row) {
                const auto page = row / PageDim, pageRow = row % PageDim;
                if (0 < page && pageRow == 0) {
                    ImGui::TableNextRow();
                    for (auto col = 0; col < Cols; ++col) {
                        ImGui::TableSetColumnIndex(col);
                        ImGui::Dummy({0, ImGui::GetTextLineHeight()});
                    }
                }
                // NOTE: since the clipper has some extra padding for the
                // page-breaks, it's possible row can go past the end of
                // ram, so guard that here.
                if (row >= totalRows) break;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                const auto rowAddr = row * PageDim;
                ImGui::Text("%04X", rowAddr);
                for (auto ramCol = 0; ramCol < PageDim; ++ramCol) {
                    renderColumn(ramCol, rowAddr, page);
                }
                ImGui::TableSetColumnIndex(Cols - 1);
                ImGui::TextUnformatted(ascii.c_str());
                std::fill(ascii.begin(), ascii.end(), Placeholder);
            }
        }

    private:
        void renderColumn(int ramCol, int rowAddr, int page)
        {
            const auto ramIdx = static_cast<aldo::et::size>(rowAddr + ramCol);
            const auto val = ram[ramIdx];
            ImGui::TableSetColumnIndex(ramCol + 1);
            if (page == 1 && ramIdx % PageSize == sp) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                       aldo::colors::LedOn);
                ImGui::TextColored(SpHighlight, "%02X", val);
            } else {
                ImGui::Text("%02X", val);
            }
            if (std::isprint(static_cast<char>(val), lcl)) {
                ascii[static_cast<ascii_sz>(ramCol)]
                    = static_cast<ascii_val>(val);
            }
        }

        const std::locale& lcl = std::locale::classic();
        std::string ascii;
        int totalRows;
        const aldo::et::byte* ram;
        aldo::et::byte sp;

        using ascii_sz = decltype(ascii)::size_type;
        using ascii_val = decltype(ascii)::value_type;
        static constexpr ascii_val Placeholder = '.';
        inline static const ImVec4 SpHighlight
            = ImGui::ColorConvertU32ToFloat4(IM_COL32_BLACK);
    };

    static constexpr int PageSize = 256, PageDim = 16, Cols = PageDim + 2;
};

}

//
// Public Interface
//

void aldo::View::render()
{
    handleTransition();
    if (!visible) return;

    if (ImGui::Begin(title.c_str(), visibility())) {
        renderContents();
    }
    ImGui::End();
}

aldo::Layout::Layout(aldo::viewstate& vs, const aldo::Emulator& emu,
                     const aldo::MediaRuntime& mr)
: vs{vs}, emu{emu}, mr{mr}
{
    add_views<
        BouncerView,
        CartInfoView,
        CpuView,
        DatapathView,
        DebuggerView,
        HardwareTraitsView,
        PrgAtPcView,
        RamView>(views, vs, emu, mr);
}

void aldo::Layout::render() const
{
    main_menu(vs, emu, mr, views);
    for (const auto& v : views) {
        v->render();
    }
    about_overlay(vs);
    if (vs.showDemo) {
        ImGui::ShowDemoWindow();
    }
}

//
// Private Interface
//

void aldo::View::handleTransition() noexcept
{
    const auto t = transition.reset();
    switch (t) {
    case aldo::View::Transition::Open:
        visible = true;
        break;
    case aldo::View::Transition::Close:
        visible = false;
        break;
    case aldo::View::Transition::Expand:
        if (visible) {
            ImGui::SetNextWindowCollapsed(false);
        }
        break;
    case aldo::View::Transition::Collapse:
        if (visible) {
            ImGui::SetNextWindowCollapsed(true);
        }
        break;
    default:
        break;
    }
}

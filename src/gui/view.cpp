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
#include "texture.hpp"
#include "version.h"
#include "viewstate.hpp"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <iterator>
#include <limits>
#include <locale>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <cinttypes>
#include <cstdio>

namespace
{

//
// MARK: - Helpers
//

// TODO: make interval a double template parameter once Apple clang supports it
class RefreshInterval {
public:
    explicit RefreshInterval(double intervalMs) noexcept
    : interval{intervalMs} {}

    bool elapsed(const cycleclock& cyclock) noexcept
    {
        if ((dt += cyclock.ticktime_ms) >= interval) {
            dt = 0;
            return true;
        }
        return false;
    }

private:
    double dt = 0, interval;
};

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

using scoped_color_int = std::pair<ImGuiCol, ImU32>;
using scoped_style_flt = std::pair<ImGuiStyleVar, float>;
using scoped_style_vec = std::pair<ImGuiStyleVar, ImVec2>;
template<typename T>
concept StyleVal = std::same_as<T, scoped_style_flt>
                    || std::same_as<T, scoped_style_vec>;
template<typename T>
concept ScopedVal = StyleVal<T> || std::same_as<T, scoped_color_int>;

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
        std::ranges::for_each(vals, Policy::push);
    }

    struct color_stack {
        static void push(const scoped_color_int& color) noexcept
        {
            ImGui::PushStyleColor(color.first, color.second);
        }
        static void pop(int count) noexcept
        {
            ImGui::PopStyleColor(count);
        }
    };
    template<StyleVal T>
    struct style_stack {
        static void push(const T& style) noexcept
        {
            ImGui::PushStyleVar(style.first, style.second);
        }
        static void pop(int count) noexcept
        {
            ImGui::PopStyleVar(count);
        }
    };
    using style_stackf = style_stack<scoped_style_flt>;
    using style_stackv = style_stack<scoped_style_vec>;
    using Policy = std::conditional_t<
                    std::same_as<V, scoped_color_int>,
                    color_stack,
                    std::conditional_t<
                        std::same_as<V, scoped_style_flt>,
                        style_stackf,
                        style_stackv>>;

    bool condition;
    typename std::initializer_list<V>::size_type count;
};
using ScopedColor = ScopedWidgetVars<scoped_color_int>;
using ScopedStyleFlt = ScopedWidgetVars<scoped_style_flt>;
using ScopedStyleVec = ScopedWidgetVars<scoped_style_vec>;

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
// MARK: - Widgets
//

using view_span = std::span<const std::unique_ptr<aldo::View>>;

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
            DisabledIf dif = !emu.debugger().isActive();
            if (ImGui::MenuItem("Export Breakpoints...", "Opt+Cmd+B")) {
                vs.commands.emplace(aldo::Command::breakpointsExport);
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Load Palette...", "Cmd+P")) {
            vs.commands.emplace(aldo::Command::paletteLoad);
        }
        {
            DisabledIf pif = emu.palette().isDefault();
            if (ImGui::MenuItem("Unload Palette", "Opt+Cmd+P")) {
                vs.commands.emplace(aldo::Command::paletteUnload);
            }
        }
        ImGui::EndMenu();
    }
}

auto speed_menu_items(aldo::viewstate& vs) noexcept
{
    static constexpr auto multiplierLabel = " 10x";

    std::string incLabel = "Cycles +", decLabel = "Cycles -";
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
    DisabledIf dif = vs.clock.cyclock.rate == MaxCps;
    if (ImGui::MenuItem(incLabel.c_str(), incKey)) {
        vs.clock.adjustCycleRate(val);
    }
    dif = vs.clock.cyclock.rate == MinCps;
    if (ImGui::MenuItem(decLabel.c_str(), decKey)) {
        vs.clock.adjustCycleRate(-val);
    }
}

auto mode_menu_item(aldo::viewstate& vs, const aldo::Emulator& emu)
{
    std::string label = "Run Mode";
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
        auto val = static_cast<csig_excmode>(emu.runMode() + modeAdjust);
        vs.commands.emplace(aldo::Command::mode, val);
    }
}

auto controls_menu(aldo::viewstate& vs, const aldo::Emulator& emu)
{
    if (ImGui::BeginMenu("Controls")) {
        speed_menu_items(vs);
        auto& lines = emu.snapshot().cpu.lines;
        if (ImGui::MenuItem(lines.ready ? "Halt" : "Run", "<Space>")) {
            vs.commands.emplace(aldo::Command::ready, !lines.ready);
        }
        mode_menu_item(vs, emu);
        ImGui::Separator();
        auto
            irq = emu.probe(CSGI_IRQ),
            nmi = emu.probe(CSGI_NMI),
            rst = emu.probe(CSGI_RST);
        if (ImGui::MenuItem("IRQ", "i", &irq)) {
            vs.addProbeCommand(CSGI_IRQ, irq);
        }
        if (ImGui::MenuItem("NMI", "n", &nmi)) {
            vs.addProbeCommand(CSGI_NMI, nmi);
        }
        if (ImGui::MenuItem("RST", "s", &rst)) {
            vs.addProbeCommand(CSGI_RST, rst);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear RAM", "Cmd+0", emu.zeroRam)) {
            vs.commands.emplace(aldo::Command::zeroRamOnPowerup, !emu.zeroRam);
        }
        ImGui::EndMenu();
    }
}

auto windows_menu(view_span views)
{
    auto viewTransition = aldo::View::Transition::None;
    if (ImGui::BeginMenu("Windows")) {
        for (auto& v : views) {
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
        for (auto& v : views) {
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
               const aldo::MediaRuntime& mr, view_span views)
{
    if (ImGui::BeginMainMenuBar()) {
        about_submenu(vs, mr);
        file_menu(vs, emu);
        controls_menu(vs, emu);
        windows_menu(views);
        tools_menu(vs);
        ImGui::EndMainMenuBar();
    }
}

auto input_address(aldo::et::word* addr) noexcept
{
    ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 6);
    ScopedID id = addr;
    return ImGui::InputScalar("Address", ImGuiDataType_U16, addr, nullptr,
                              nullptr, "%04X");
}

auto about_overlay(aldo::viewstate& vs) noexcept
{
    if (!vs.showAbout) return;

    static constexpr auto flags = ImGuiWindowFlags_AlwaysAutoResize
                                    | ImGuiWindowFlags_NoDecoration
                                    | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoNav
                                    | ImGuiWindowFlags_NoSavedSettings;

    auto workArea = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(workArea + 25);
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
// MARK: - Concrete Views
//

class BouncerView final : public aldo::View {
public:
    BouncerView(aldo::viewstate& vs, const aldo::Emulator& emu,
                const aldo::MediaRuntime& mr)
    : View{"Bouncer", vs, emu, mr}, bouncer{vs.bouncer.bounds, mr.renderer()}
    {}
    BouncerView(aldo::viewstate&, aldo::Emulator&&,
                const aldo::MediaRuntime&) = delete;
    BouncerView(aldo::viewstate&, const aldo::Emulator&,
                aldo::MediaRuntime&&) = delete;
    BouncerView(aldo::viewstate&, aldo::Emulator&&,
                aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        bouncer.draw(vs, mr.renderer());
        bouncer.render();
    }

private:
    aldo::BouncerScreen bouncer;
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

        auto
            textSz = aldo::style::glyph_size(),
            availSpace = ImGui::GetContentRegionAvail();
        auto nameFit = static_cast<int>((availSpace.x / textSz.x)
                                        - label.length());
        auto name = emu.displayCartName();

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

        auto info = emu.cartInfo();
        if (info) {
            ImGui::Text("Format: %s", cart_formatname(info->format));
            ImGui::Separator();
            if (info->format == CRTF_INES) {
                renderiNesInfo(info.value());
            } else {
                renderRawInfo();
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
        if (ImGui::CollapsingHeader("Datapath",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            renderDatapath();
        }
    }

private:
    void renderRegisters() const
    {
        auto& cpu = emu.snapshot().cpu;
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

        auto textSz = aldo::style::glyph_size();
        auto radius = (textSz.x + textSz.y) / 2;
        auto pos = ImGui::GetCursorScreenPos();
        ImVec2 center = pos + radius;

        auto fontSz = ImGui::GetFontSize();
        ImVec2 offset{fontSz / 4, fontSz / 2};
        auto drawList = ImGui::GetWindowDrawList();

        for (auto it = flags.cbegin(); it != flags.cend(); ++it) {
            auto bitpos = std::distance(it, flags.cend()) - 1;
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

    void renderDatapath() const noexcept
    {
        auto glyphW = aldo::style::glyph_size().x, lineSpacer = glyphW * 8;
        renderControlLines(lineSpacer, glyphW);
        ImGui::Separator();
        renderBusLines();
        ImGui::Separator();
        renderInstructionDecode();
        ImGui::Separator();
        renderInterruptLines(lineSpacer);
    }

    void renderControlLines(float spacer, float adjustW) const noexcept
    {
        auto& lines = emu.snapshot().cpu.lines;
        renderControlLine("RDY", lines.ready);
        ImGui::SameLine(0, spacer);
        renderControlLine("SYNC", lines.sync);
        // NOTE: adjust spacer width for SYNC's extra letter
        ImGui::SameLine(0, spacer - adjustW);
        renderRWLine(lines.readwrite, adjustW);
    }

    void renderBusLines() const noexcept
    {
        auto& datapath = emu.snapshot().cpu.datapath;
        {
            ScopedColor color{{ImGuiCol_Text, aldo::colors::LineOut}};
            ImGui::Text("Addr: %04X", datapath.addressbus);
        };
        ImGui::SameLine(0, 20);
        if (datapath.busfault) {
            ScopedColor color{
                {ImGuiCol_Text, aldo::colors::DestructiveHover},
            };
            ImGui::TextUnformatted("Data: FLT");
        } else {
            std::array<char, 3> dataHex;
            std::snprintf(dataHex.data(), dataHex.size(), "%02X",
                          datapath.databus);
            ScopedColor color{{
                ImGuiCol_Text,
                emu.snapshot().cpu.lines.readwrite
                    ? aldo::colors::LineIn
                    : aldo::colors::LineOut,
            }};
            ImGui::Text("Data: %s", dataHex.data());
        }
    }

    void renderInstructionDecode() const noexcept
    {
        auto& datapath = emu.snapshot().cpu.datapath;
        if (datapath.jammed) {
            ScopedColor color{{ImGuiCol_Text, aldo::colors::DestructiveHover}};
            ImGui::TextUnformatted("Decode: JAMMED");
        } else {
            std::array<aldo::et::tchar, DIS_DATAP_SIZE> buf;
            auto err = dis_datapath(emu.snapshotp(), buf.data());
            ImGui::Text("Decode: %s", err < 0 ? dis_errstr(err) : buf.data());
        }
        ImGui::Text("adl: %02X", datapath.addrlow_latch);
        ImGui::Text("adh: %02X", datapath.addrhigh_latch);
        ImGui::Text("adc: %02X", datapath.addrcarry_latch);
        renderCycleIndicator(datapath.exec_cycle);
    }

    void renderInterruptLines(float spacer) const noexcept
    {
        ImGui::Spacing();

        auto& lines = emu.snapshot().cpu.lines;
        renderInterruptLine("IRQ", lines.irq);
        ImGui::SameLine(0, spacer);
        renderInterruptLine("NMI", lines.nmi);
        ImGui::SameLine(0, spacer);
        renderInterruptLine("RST", lines.reset);

        auto& datapath = emu.snapshot().cpu.datapath;
        ImGui::TextUnformatted(display_signalstate(datapath.irq));
        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted(display_signalstate(datapath.nmi));
        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted(display_signalstate(datapath.rst));
    }

    static void renderCycleIndicator(aldo::et::byte cycle) noexcept
    {
        static constexpr auto radius = 5;

        ImGui::TextUnformatted("t:");
        ImGui::SameLine();
        auto pos = ImGui::GetCursorScreenPos();
        ImVec2 center{pos.x, pos.y + (ImGui::GetTextLineHeight() / 2) + 1};
        auto drawList = ImGui::GetWindowDrawList();
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
        DisabledIf dif = !active;
        ImGui::TextUnformatted(label);
    }

    static void renderRWLine(bool active, float adjustW) noexcept
    {
        DisabledIf dif = !active;
        ImGui::TextUnformatted("R/W");
        ImVec2
            end{ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y},
            start{end.x - adjustW, end.y};
        auto drawList = ImGui::GetWindowDrawList();
        drawList->AddLine(start, end, aldo::colors::white(active));
    }

    static void renderInterruptLine(const char* label, bool active) noexcept
    {
        DisabledIf dif = !active;
        ImGui::TextUnformatted(label);
        auto start = ImGui::GetItemRectMin();
        ImVec2 end{
            start.x + ImGui::GetItemRectSize().x, start.y,
        };
        auto drawlist = ImGui::GetWindowDrawList();
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

        std::ranges::generate(haltConditions,
            [h = static_cast<halt_integral>(HLT_NONE)] mutable -> halt_val {
                auto cond = static_cast<halt_val::first_type>(++h);
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
        std::array<std::pair<haltcondition, aldo::et::str>, HLT_COUNT - 1>;
    using halt_it = halt_array::const_iterator;
    using bp_sz = aldo::Debugger::BpView::size_type;
    using bp_diff = aldo::Debugger::BreakpointIterator::difference_type;

    class SelectedBreakpoints {
    public:
        bool empty() const noexcept { return selections.empty(); }
        bp_diff mostRecent() const
        {
            return empty() ? NoSelection : selections.back();
        }
        bool selected(bp_diff idx) const
        {
            return std::ranges::any_of(selections,
                                       [idx](bp_diff i) { return i == idx; });
        }

        void select(bp_diff idx)
        {
            clear();
            selections.push_back(idx);
        }
        void multiSelect(bp_diff idx)
        {
            if (selected(idx)) {
                std::erase(selections, idx);
            } else {
                selections.push_back(idx);
            }
        }
        void rangeSelect(bp_diff idx)
        {
            auto lastSelection = mostRecent();
            if (lastSelection == NoSelection) {
                select(idx);
            } else {
                auto [low, high] = std::minmax(idx, lastSelection);
                for (auto i = low; i <= high; ++i) {
                    selections.push_back(i);
                }
                // NOTE: add current idx last to mark it as the
                // most recent selection.
                selections.push_back(idx);
            }
        }
        void clear() noexcept { selections.clear(); }

        void queueEnableToggles(aldo::viewstate& vs, bool enabled) const
        {
            for (auto idx : selections) {
                auto cmd = enabled
                            ? aldo::Command::breakpointDisable
                            : aldo::Command::breakpointEnable;
                vs.commands.emplace(cmd, idx);
            }
        }
        void queueRemovals(aldo::viewstate& vs) const
        {
            // NOTE: queue remove commands in descending order to avoid
            // invalidating bp indices during removal.
            decltype(selections) sorted(selections.size());
            std::ranges::partial_sort_copy(selections, sorted,
                                           std::ranges::greater{});
            std::unordered_set<bp_diff> removed(sorted.size());
            for (auto idx : sorted) {
                // NOTE: here's where duplicate selections bite us
                if (removed.contains(idx)) continue;
                vs.commands.emplace(aldo::Command::breakpointRemove, idx);
                removed.insert(idx);
            }
        }

    private:
        // NOTE: arguably this should be a set but the ordered property of
        // vector gives saner multi-select behavior when mixing shift and cmd
        // clicks where the last element can be treated as the most recent
        // selection; duplicates are handled properly with std::erase.
        std::vector<bp_diff> selections;
    };

    void renderVectorOverride()
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
        DisabledIf dif = [this] {
            if (this->resetOverride) return false;
            // NOTE: +2 = start of reset vector
            this->resetAddr = batowr(this->emu.snapshot().prg.vectors + 2);
            return true;
        }();
        if (input_address(&resetAddr)) {
            vs.commands.emplace(aldo::Command::resetVectorOverride,
                                static_cast<int>(resetAddr));
        }
    }

    void renderBreakpoints()
    {
        auto setFocus = renderConditionCombo();
        ImGui::Separator();
        renderBreakpointAdd(setFocus);
        ImGui::Separator();
        renderBreakpointList();
        detectedHalt = emu.debugger().halted();
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
                auto current = it == selectedCondition;
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

    void renderBreakpointAdd(bool setFocus)
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
        auto submitted = ImGui::IsItemDeactivated() && enter_pressed();
        if (ImGui::Button("Add") || submitted) {
            vs.commands.emplace(aldo::Command::breakpointAdd,
                                currentHaltExpression);
        }
    }

    void renderBreakpointList()
    {
        ImVec2 dims{
            -std::numeric_limits<float>::min(),
            8 * ImGui::GetTextLineHeightWithSpacing(),
        };
        auto bpView = emu.debugger().breakpoints();
        auto breakIdx = bpView.halted_at();
        auto bpCount = bpView.size();
        ImGui::Text("%zu breakpoint%s", bpCount, bpCount == 1 ? "" : "s");
        if (ImGui::BeginListBox("##breakpoints", dims)) {
            bp_diff i = 0;
            for (auto& bp : bpView) {
                renderBreakpoint(i++, breakIdx, bp);
            }
            ImGui::EndListBox();
        }
        renderListControls(bpCount);
    }

    void renderBreakpoint(bp_diff idx, bp_diff breakIdx, const breakpoint& bp)
    {
        auto bpBreak = idx == breakIdx;
        if (bpBreak && !detectedHalt) {
            bpSelections.select(idx);
        }
        std::array<aldo::et::tchar, HEXPR_FMT_SIZE> fmt;
        auto err = haltexpr_desc(&bp.expr, fmt.data());
        ScopedStyleFlt style{
            {ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha},
            !bp.enabled,
        };
        ScopedColor color{
            {ImGuiCol_Text, aldo::colors::Attention}, bpBreak,
        };
        ScopedID id = static_cast<int>(idx);
        if (ImGui::Selectable(err < 0 ? haltexpr_errstr(err) : fmt.data(),
                              bpSelections.selected(idx))) {
            if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
                bpSelections.rangeSelect(idx);
            } else if (ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
                bpSelections.multiSelect(idx);
            } else {
                bpSelections.select(idx);
            }
        }
    }

    void renderListControls(bp_sz bpCount)
    {
        DisabledIf dif = bpSelections.empty();
        auto& dbg = emu.debugger();
        auto bp = dbg.breakpoints().at(bpSelections.mostRecent());
        if (ImGui::Button(!bp || bp->enabled ? "Disable" : "Enable ")) {
            bpSelections.queueEnableToggles(vs, bp->enabled);
        }
        ImGui::SameLine();
        ScopedColor colors{
            {ImGuiCol_Button, aldo::colors::Destructive},
            {ImGuiCol_ButtonHovered, aldo::colors::DestructiveHover},
            {ImGuiCol_ButtonActive, aldo::colors::DestructiveActive},
        };
        auto resetSelection = false;
        if (ImGui::Button("Remove")) {
            bpSelections.queueRemovals(vs);
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
        auto& dbg = emu.debugger();
        if ((resetOverride = dbg.isVectorOverridden())) {
            resetAddr = static_cast<aldo::et::word>(dbg.vectorOverride());
        }
    }

    bool detectedHalt = false, resetOverride = false;
    aldo::et::word resetAddr = 0x0;
    halt_array haltConditions;
    // NOTE: storing iterator is safe as haltConditions
    // is immutable for the life of this instance.
    halt_it selectedCondition;
    haltexpr currentHaltExpression;
    SelectedBreakpoints bpSelections;
};

class PaletteView final : public aldo::View {
public:
    PaletteView(aldo::viewstate& vs, const aldo::Emulator& emu,
                const aldo::MediaRuntime& mr) noexcept
    : View{"Palette", vs, emu, mr} {}
    PaletteView(aldo::viewstate&, aldo::Emulator&&,
                const aldo::MediaRuntime&) = delete;
    PaletteView(aldo::viewstate&, const aldo::Emulator&,
                aldo::MediaRuntime&&) = delete;
    PaletteView(aldo::viewstate&, aldo::Emulator&&,
                aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        static constexpr auto flags = ImGuiTableFlags_Borders
                                        | ImGuiTableFlags_SizingFixedFit;
        auto name = emu.palette().name();
        ImGui::Text("%.*s", static_cast<int>(name.length()), name.data());
        if (ImGui::BeginTable("palette", Cols, flags)) {
            renderHeader();
            renderBody();
            ImGui::EndTable();
        }
        renderDetails();
    }

private:
    using pal_sz = aldo::palette::sz;

    static void renderHeader() noexcept
    {
        std::array<char, 3> buf;
        ImGui::TableSetupColumn("Idx", ImGuiTableColumnFlags_WidthStretch);
        for (pal_sz col = 1; col < Cols; ++col) {
            std::snprintf(buf.data(), buf.size(), " %01zX", col - 1);
            ImGui::TableSetupColumn(buf.data());
        }
        ImGui::TableHeadersRow();
    }

    void renderBody()
    {
        static constexpr auto cellDim = 15;
        static constexpr pal_sz rows = 4,
                                paletteDim = aldo::palette::Size / rows;
        static constexpr auto center = 0.5f;

        ScopedStyleVec textAlign{{
            ImGuiStyleVar_SelectableTextAlign,
            {center, center},
        }};
        for (pal_sz row = 0; row < rows; ++row) {
            for (pal_sz col = 0; col < Cols; ++col) {
                auto rowStart = paletteDim * row;
                ImGui::TableNextColumn();
                if (col == 0) {
                    ImGui::Text(" %02zX", rowStart);
                } else {
                    auto cell = rowStart + (col - 1);
                    auto color = emu.palette().getColor(adjustIdx(cell),
                                                        colorEmphasis);
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
                    ScopedColor indicatorColor{
                        {ImGuiCol_Text, aldo::colors::luminance(color) < 0x80
                                        ? IM_COL32_WHITE
                                        : IM_COL32_BLACK},
                        cell == vs.colorSelection,
                    };
                    ScopedID id = static_cast<int>(cell);
                    if (ImGui::Selectable(cell == vs.colorSelection ? "x" : "",
                                          false, ImGuiSelectableFlags_None,
                                          {cellDim, cellDim})) {
                        vs.colorSelection = cell;
                    }
                }
            }
        }
    }

    void renderDetails()
    {
        renderColorMods();
        ImGui::SameLine();
        renderColorSelection();
    }

    void renderColorMods()
    {
        widget_group([this] {
            ImGui::Checkbox("Red Emphasis", &this->colorEmphasis.r);
            ImGui::Checkbox("Green Emphasis", &this->colorEmphasis.g);
            ImGui::Checkbox("Blue Emphasis", &this->colorEmphasis.b);
        });
        ImGui::SameLine();
        ImGui::Checkbox("Grayscale", &grayscale);
    }

    void renderColorSelection() const
    {
        static constexpr auto selectedColorDim = 70;
        auto color = emu.palette().getColor(adjustIdx(vs.colorSelection),
                                            colorEmphasis);
        ImGui::ColorButton("Selected Color",
                           ImGui::ColorConvertU32ToFloat4(color),
                           ImGuiColorEditFlags_NoAlpha,
                           {selectedColorDim, selectedColorDim});
        ImGui::SameLine();
        widget_group([color, &vs = vs] {
            auto [r, g, b] = aldo::colors::rgb(color);
            ImGui::Text("Index: %02zX", vs.colorSelection);
            if (vs.colorSelection == 0xd) {
                ImGui::SameLine();
                ImGui::TextUnformatted("(forbidden black)");
            }
            ImGui::Text("RGBd:  %d %d %d", r, g, b);
            ImGui::Text("RGBx:  %02X %02X %02X", r, g, b);
            ImGui::Text("Hex:   #%02x%02x%02x", r, g, b);
        });
    }

    pal_sz adjustIdx(pal_sz idx) const noexcept
    {
        return grayscale ? idx & 0x30 : idx;
    }

    bool grayscale = false;
    aldo::palette::emphasis colorEmphasis = {};

    static constexpr pal_sz Cols = 17;
};

class PatternTablesView final : public aldo::View {
public:
    PatternTablesView(aldo::viewstate& vs, const aldo::Emulator& emu,
                      const aldo::MediaRuntime& mr)
    : View{"Pattern Tables", vs, emu, mr},
        left{mr.renderer()}, right{mr.renderer()} {}
    PatternTablesView(aldo::viewstate&, aldo::Emulator&&,
                      const aldo::MediaRuntime&) = delete;
    PatternTablesView(aldo::viewstate&, const aldo::Emulator&,
                      aldo::MediaRuntime&&) = delete;
    PatternTablesView(aldo::viewstate&, aldo::Emulator&&,
                      aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (drawInterval.elapsed(vs.clock.cyclock)) {
            const auto* tables = &emu.snapshot().video->pattern_tables;
            // TODO: make this selectable
            const auto* colors = emu.snapshot().video->palettes.bg[0];
            left.draw(tables->left, colors, emu.palette());
            right.draw(tables->right, colors, emu.palette());
        }

        widget_group([this] {
            ImGui::TextUnformatted("Pattern Table $0000");
            this->left.render();
        });
        ImGui::SameLine(0, 10);
        widget_group([this] {
            ImGui::TextUnformatted("Pattern Table $1000");
            this->right.render();
        });
        ImGui::SameLine(0, 10);
        widget_group([this] {
            this->renderPalettes(false);
            this->renderPalettes(true);
        });
    }

private:
    void renderPalettes(bool fg)
    {
        static constexpr auto cellDim = 15;
        static constexpr auto cols = CHR_PAL_SIZE + 1;
        static constexpr auto center = 0.5f;
        static constexpr auto flags = ImGuiTableFlags_BordersInner
                                        | ImGuiTableFlags_SizingFixedFit;

        ImGui::TextUnformatted(fg ? "Sprites" : "Background");
        const auto* pals = fg
                            ? emu.snapshot().video->palettes.fg
                            : emu.snapshot().video->palettes.bg;
        for (auto row = 0; row < CHR_PAL_SIZE; ++row) {
            if (ImGui::BeginTable(fg ? "FgPalettes" : "BgPalettes", cols,
                                  flags)) {
                ScopedStyleVec textAlign{{
                    ImGuiStyleVar_SelectableTextAlign, {center, center},
                }};
                auto pal = pals[row];
                for (auto col = 0; col < cols; ++col) {
                    ImGui::TableNextColumn();
                    if (col == 0) {
                        ImGui::Text("%d", row + 1);
                    } else {
                        auto idx = pal[col - 1];
                        auto color = emu.palette().getColor(idx);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                               color);
                        std::array<char, 3> buf;
                        std::snprintf(buf.data(), buf.size(), "%02X", idx);
                        ScopedColor txtColor{{
                            ImGuiCol_Text,
                            aldo::colors::luminance(color) < 0x80
                                ? IM_COL32_WHITE
                                : IM_COL32_BLACK,
                        }};
                        ScopedID id = row << 3 | col << 1 | fg;
                        if (ImGui::Selectable(buf.data(), false,
                                              ImGuiSelectableFlags_None,
                                              {cellDim, cellDim})) {
                            vs.colorSelection = idx;
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
    }

    aldo::PatternTable left, right;
    RefreshInterval drawInterval{250};
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

        const auto* curr = emu.snapshot().prg.curr;
        auto addr = emu.snapshot().cpu.datapath.current_instruction;
        dis_instruction inst{};
        std::array<aldo::et::tchar, DIS_INST_SIZE> disasm;
        for (int i = 0; i < instCount; ++i) {
            auto result = dis_parsemem_inst(curr->length, curr->pc,
                                            inst.offset + inst.bv.size, &inst);
            if (result > 0) {
                result = dis_inst(addr, &inst, disasm.data());
                if (result > 0) {
                    if (ImGui::Selectable(disasm.data(), selected == i)) {
                        selected = i;
                    } else if (ImGui::BeginPopupContextItem()) {
                        selected = i;
                        if (ImGui::Selectable("Add breakpoint...")) {
                            auto expr = haltexpr{
                                .address = addr, .cond = HLT_ADDR,
                            };
                            vs.commands.emplace(aldo::Command::breakpointAdd,
                                                expr);
                        }
                        ImGui::EndPopup();
                    }
                    addr += static_cast<aldo::et::word>(inst.bv.size);
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
        auto& prg = emu.snapshot().prg;
        auto
            lo = prg.vectors[0],
            hi = prg.vectors[1];
        ImGui::Text("%04X: %02X %02X     NMI $%04X", CPU_VECTOR_NMI, lo, hi,
                    bytowr(lo, hi));

        lo = prg.vectors[2];
        hi = prg.vectors[3];
        const char* indicator;
        aldo::et::word resVector;
        auto& dbg = emu.debugger();
        if (dbg.isVectorOverridden()) {
            indicator = HEXPR_RST_IND;
            resVector = static_cast<aldo::et::word>(dbg.vectorOverride());
        } else {
            indicator = "";
            resVector = bytowr(lo, hi);
        }
        ImGui::Text("%04X: %02X %02X     RST %s$%04X", CPU_VECTOR_RST, lo, hi,
                    indicator, resVector);

        lo = prg.vectors[4];
        hi = prg.vectors[5];
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

        auto
            pageCount = static_cast<int>(emu.ramSize()) / PageSize,
            rowCount = pageCount * PageDim;
        ImVec2 tableSize{
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
                auto page = row / PageDim, pageRow = row % PageDim;
                if (0 < page && pageRow == 0) {
                    for (auto col = 0; col < Cols; ++col) {
                        ImGui::TableNextColumn();
                        ImGui::Dummy({0, ImGui::GetTextLineHeight()});
                    }
                }
                // NOTE: since the clipper has some extra padding for the
                // page-breaks, it's possible row can go past the end of
                // ram, so guard that here.
                if (row >= totalRows) break;

                auto rowAddr = row * PageDim;
                ImGui::TableNextColumn();
                ImGui::Text("%04X", rowAddr);
                for (auto ramCol = 0; ramCol < PageDim; ++ramCol) {
                    renderColumn(ramCol, rowAddr, page);
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ascii.c_str());
                std::ranges::fill(ascii, Placeholder);
            }
        }

    private:
        void renderColumn(int ramCol, int rowAddr, int page)
        {
            auto ramIdx = static_cast<aldo::et::size>(rowAddr + ramCol);
            auto val = ram[ramIdx];
            ImGui::TableNextColumn();
            if (page == 1 && ramIdx % PageSize == sp) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                       aldo::colors::LedOn);
                ImGui::TextColored(SpHighlight, "%02X", val);
            } else {
                ImGui::Text("%02X", val);
            }
            if (std::isprint(static_cast<char>(val), lcl)) {
                ascii[static_cast<ascii_sz>(ramCol)] =
                    static_cast<ascii_val>(val);
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
        inline static const ImVec4 SpHighlight =
            ImGui::ColorConvertU32ToFloat4(IM_COL32_BLACK);
    };

    static constexpr int
        PageDim = 16, PageSize = PageDim * PageDim, Cols = PageDim + 2;
};

class SystemView final : public aldo::View {
public:
    SystemView(aldo::viewstate& vs, const aldo::Emulator& emu,
               const aldo::MediaRuntime& mr) noexcept
    : View{"System", vs, emu, mr} {}
    SystemView(aldo::viewstate&, aldo::Emulator&&,
               const aldo::MediaRuntime&) = delete;
    SystemView(aldo::viewstate&, const aldo::Emulator&,
               aldo::MediaRuntime&&) = delete;
    SystemView(aldo::viewstate&, aldo::Emulator&&,
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
        const auto& cyclock = vs.clock.cyclock;
        if (statsInterval.elapsed(cyclock)) {
            dispDtInput = vs.clock.dtInputMs;
            dispDtUpdate = vs.clock.dtUpdateMs;
            dispDtRender = vs.clock.dtRenderMs;
            dispDtElapsed = dispDtInput + dispDtUpdate + dispDtRender;
            dispDtTick = cyclock.ticktime_ms;
        }
        ImGui::Text("Input dT: %.3f", dispDtInput);
        ImGui::Text("Update dT: %.3f", dispDtUpdate);
        ImGui::Text("Render dT: %.3f", dispDtRender);
        ImGui::Text("Elapsed dT: %.3f", dispDtElapsed);
        ImGui::Text("Tick dT: %.3f (%+.3f)", dispDtTick,
                    dispDtTick - dispDtElapsed);
        ImGui::Text("Ticks: %" PRIu64, cyclock.ticks);
        ImGui::Text("Emutime: %.3f", cyclock.emutime);
        ImGui::Text("Runtime: %.3f", cyclock.runtime);
        ImGui::Text("Cycles: %" PRIu64, cyclock.cycles);
        ImGui::Text("Frames: %" PRIu64, cyclock.frames);
        ImGui::Text("BCD Support: %s", boolstr(emu.bcdSupport()));
    }

    void renderSpeedControls() const noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Cycles/Second");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::DragInt("##cyclesPerSecond", &vs.clock.cyclock.rate, 1, MinCps,
                       MaxCps, "%d", ImGuiSliderFlags_AlwaysClamp);
    }

    void renderRunControls() const
    {
        auto& lines = emu.snapshot().cpu.lines;
        auto halt = !lines.ready;
        if (ImGui::Checkbox("HALT", &halt)) {
            vs.commands.emplace(aldo::Command::ready, !halt);
        };

        auto mode = emu.runMode();
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
        auto
            irq = emu.probe(CSGI_IRQ),
            nmi = emu.probe(CSGI_NMI),
            rst = emu.probe(CSGI_RST);
        if (ImGui::Checkbox("IRQ", &irq)) {
            vs.addProbeCommand(CSGI_IRQ, irq);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("NMI", &nmi)) {
            vs.addProbeCommand(CSGI_NMI, nmi);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("RST", &rst)) {
            vs.addProbeCommand(CSGI_RST, rst);
        }
    }

    double
        dispDtInput = 0, dispDtUpdate = 0, dispDtRender = 0, dispDtElapsed = 0,
        dispDtTick = 0;
    RefreshInterval statsInterval{250};
};

}

//
// MARK: - Public Interface
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
        DebuggerView,
        PaletteView,
        PatternTablesView,
        PrgAtPcView,
        RamView,
        SystemView>(views, vs, emu, mr);
}

void aldo::Layout::render() const
{
    main_menu(vs, emu, mr, views);
    for (auto& v : views) {
        v->render();
    }
    about_overlay(vs);
    if (vs.showDemo) {
        ImGui::ShowDemoWindow();
    }
}

//
// MARK: - Private Interface
//

void aldo::View::handleTransition() noexcept
{
    auto t = transition.reset();
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

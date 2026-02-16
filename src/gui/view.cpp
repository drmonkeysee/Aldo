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
#include "ctrlsignal.h"
#include "cycleclock.h"
#include "debug.hpp"
#include "dis.h"
#include "emu.hpp"
#include "emutypes.hpp"
#include "haltexpr.h"
#include "mediaruntime.hpp"
#include "render.hpp"
#include "style.hpp"
#include "texture.hpp"
#include "version.h"
#include "viewstate.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <iterator>
#include <limits>
#include <locale>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <cassert>
#include <cinttypes>
#include <cstdio>

namespace
{

//
// MARK: - Helpers
//

constexpr auto operator+(const ImVec2& v, float f) noexcept
{
    return ImVec2{v.x + f, v.y + f};
}

constexpr auto operator+(const ImVec2& a, const ImVec2& b) noexcept
{
    return ImVec2{a.x + b.x, a.y + b.y};
}

constexpr auto operator-(const ImVec2& a, const ImVec2& b) noexcept
{
    return ImVec2{a.x - b.x, a.y - b.y};
}

constexpr auto point_to_vec(const SDL_Point& p) noexcept
{
    return ImVec2{static_cast<float>(p.x), static_cast<float>(p.y)};
}

constexpr auto vec_to_pointf(const ImVec2& v) noexcept
{
    return SDL_FPoint{v.x, v.y};
}

constexpr auto vecs_to_rectf(const ImVec2& point, const ImVec2& size) noexcept
{
    return SDL_FRect{point.x, point.y, size.x, size.y};
}

constexpr auto operator-(const SDL_FPoint& p, const ImVec2& v) noexcept
{
    return SDL_FPoint{p.x - v.x, p.y - v.y};
}

template<double Interval>
class RefreshInterval {
public:
    bool elapsed(const aldo_clock& clock) noexcept
    {
        if ((dt += clock.ticktime_ms) >= Interval) {
            dt = 0;
            return true;
        }
        return false;
    }

private:
    double dt = 0;
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
    // This works because internally ImGui::Disabled is a stack, so the
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

[[maybe_unused]]
void swap(DisabledIf& a, DisabledIf& b) noexcept
{
    a.swap(b);
}

template<typename T>
concept ScopedIDVal = std::convertible_to<T, void*> || std::convertible_to<T, int>;

class ALDO_SIDEFX ScopedID {
public:
    ScopedID(ScopedIDVal auto id) noexcept { ImGui::PushID(id); }
    ScopedID(const ScopedID&) = delete;
    ScopedID& operator=(const ScopedID&) = delete;
    ScopedID(ScopedID&&) = delete;
    ScopedID& operator=(ScopedID&&) = delete;
    ~ScopedID() { ImGui::PopID(); }
};

class ALDO_SIDEFX ClipRect {
public:
    ClipRect(const ImVec2& origin, const ImVec2& size) noexcept
    : drawList{ImGui::GetWindowDrawList()}
    {
        drawList->PushClipRect(origin, origin + size);
    }
    ClipRect(const ClipRect&) = delete;
    ClipRect& operator=(const ClipRect&) = delete;
    ClipRect(ClipRect&&) = delete;
    ClipRect& operator=(ClipRect&&) = delete;
    ~ClipRect() { drawList->PopClipRect(); }

private:
    ImDrawList* drawList;
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

constexpr auto signal_label(aldo_sigstate s) noexcept
{
    switch (s) {
    case ALDO_SIG_DETECTED:
        return "D";
    case ALDO_SIG_PENDING:
        return "P";
    case ALDO_SIG_COMMITTED:
        return "C";
    case ALDO_SIG_SERVICED:
        return "S";
    default:
        return "O";
    }
}

constexpr auto signal_color(aldo_sigstate s) noexcept
{
    switch (s) {
    case ALDO_SIG_DETECTED:
        return aldo::colors::Attention;
    case ALDO_SIG_PENDING:
        return aldo::colors::LedOn;
    case ALDO_SIG_COMMITTED:
        return aldo::colors::LineOut;
    case ALDO_SIG_SERVICED:
        return aldo::colors::LineIn;
    default:
        return aldo::colors::LedOff;
    }
}

constexpr auto boolstr(bool v) noexcept
{
    return v ? "yes" : "no";
}

constexpr auto text_contrast(ImU32 fillColor) noexcept
{
    return aldo::colors::luminance(fillColor) < 0x80
            ? IM_COL32_WHITE
            : IM_COL32_BLACK;
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

template<typename V>
class ComboList {
public:
    using option = std::pair<V, const char*>;
    using callback = std::function<void(V, bool)>;

    ComboList(const char* label, std::ranges::range auto&& opts, callback cb)
    : label{label}, options{
        std::ranges::to<
            decltype(this->options)>(std::forward<decltype(opts)>(opts)),
    },
    selected{this->options.front()}, onSelected{cb}
    {
        onSelected(selection(), true);
    }

    ComboList(const char* label, std::initializer_list<option> opts, callback cb)
    : label{label}, options{opts}, selected{this->options.front()},
    onSelected{cb}
    {
        onSelected(selection(), true);
    }

    void render() noexcept
    {
        if (ImGui::BeginCombo(label, selected.second)) {
            for (const auto& option : options) {
                auto current = option == selected;
                if (ImGui::Selectable(option.second, current)) {
                    selected = option;
                    onSelected(selected.first, !current);
                }
            }
            ImGui::EndCombo();
        }
    }

    V selection() const noexcept { return selected.first; }

private:
    const char* label;
    std::vector<option> options;
    option selected;
    callback onSelected;
};

auto widget_group(std::invocable auto f) noexcept(noexcept(f()))
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
    static constexpr auto multiplierLabel = " 10x", labelBase = "Clock Rate ";

    std::string incLabel = labelBase, decLabel = labelBase;
    incLabel += "+";
    decLabel += "-";
    const char* incKey, *decKey;
    int val;
    if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
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
    {
        DisabledIf dif = vs.clock.atMaxRate();
        if (ImGui::MenuItem(incLabel.c_str(), incKey)) {
            vs.clock.adjustRate(val);
        }
        dif = vs.clock.atMinRate();
        if (ImGui::MenuItem(decLabel.c_str(), decKey)) {
            vs.clock.adjustRate(-val);
        }
    }
    if (ImGui::MenuItem("Clock Scale", "c")) {
        vs.clock.toggleScale();
    }
}

auto mode_menu_item(aldo::viewstate& vs, const aldo::Emulator& emu)
{
    std::string label = "Run Mode";
    const char* mnemonic;
    int modeAdjust;
    if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
        label += " (R)";
        mnemonic = "M";
        modeAdjust = -1;
    } else {
        mnemonic = "m";
        modeAdjust = 1;
    }
    if (ImGui::MenuItem(label.c_str(), mnemonic)) {
        auto val = static_cast<aldo_execmode>(emu.runMode() + modeAdjust);
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
            irq = emu.probe(ALDO_INT_IRQ),
            nmi = emu.probe(ALDO_INT_NMI),
            rst = emu.probe(ALDO_INT_RST);
        if (ImGui::MenuItem("IRQ", "i", &irq)) {
            vs.addProbeCommand(ALDO_INT_IRQ, irq);
        }
        if (ImGui::MenuItem("NMI", "n", &nmi)) {
            vs.addProbeCommand(ALDO_INT_NMI, nmi);
        }
        if (ImGui::MenuItem("RST", "s", &rst)) {
            vs.addProbeCommand(ALDO_INT_RST, rst);
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
    auto viewTransition = aldo::View::Transition::none;
    if (ImGui::BeginMenu("Windows")) {
        for (auto& v : views) {
            ImGui::MenuItem(v->windowTitle().c_str(), nullptr,
                            v->visibility());
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Open all")) {
            viewTransition = aldo::View::Transition::open;
        }
        if (ImGui::MenuItem("Close all")) {
            viewTransition = aldo::View::Transition::close;
        }
        if (ImGui::MenuItem("Expand all")) {
            viewTransition = aldo::View::Transition::expand;
        }
        if (ImGui::MenuItem("Collapse all")) {
            viewTransition = aldo::View::Transition::collapse;
        }
        ImGui::EndMenu();
    }
    if (viewTransition != aldo::View::Transition::none) {
        for (auto& v : views) {
            v->transition = viewTransition;
        }
    }
}

auto tools_menu(aldo::viewstate& vs)
{
    if (ImGui::BeginMenu("Tools")) {
        ImGui::MenuItem("ImGui Demo", "Cmd+D", &vs.showDemo);
        ImGui::MenuItem("Design Palette", nullptr, &vs.showDesignPalette);
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
        ImGui::Text("Aldo %s", Aldo_Version);
    #ifdef __VERSION__
        ImGui::TextUnformatted(__VERSION__);
    #endif
        auto v = SDL_GetVersion();
        ImGui::Text("SDL %u.%u.%u (%s)", SDL_VERSIONNUM_MAJOR(v),
                    SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v),
                    SDL_GetRevision());
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

auto design_palette(aldo::viewstate& vs) noexcept
{
    if (!vs.showDesignPalette) return;

    auto cbutton = [](const char* lbl, ImU32 c) static noexcept {
        ImGui::ColorButton(lbl, ImGui::ColorConvertU32ToFloat4(c),
                           ImGuiColorEditFlags_NoDragDrop);
        ImGui::SameLine();
        ImGui::TextUnformatted(lbl);
    };

    if (ImGui::Begin("Design Palette", &vs.showDesignPalette)) {
        auto screenFill = IM_COL32(aldo::colors::ScreenFill,
                                   aldo::colors::ScreenFill,
                                   aldo::colors::ScreenFill,
                                   SDL_ALPHA_OPAQUE);
        cbutton("Screen Fill", screenFill);
        cbutton("Attention", aldo::colors::Attention);
        cbutton("Destructive", aldo::colors::Destructive);
        cbutton("Destructive Active", aldo::colors::DestructiveActive);
        cbutton("Destructive Hover", aldo::colors::DestructiveHover);
        cbutton("Led Off", aldo::colors::LedOff);
        cbutton("Led On", aldo::colors::LedOn);
        cbutton("Line In", aldo::colors::LineIn);
        cbutton("Line Out", aldo::colors::LineOut);
    }
    ImGui::End();
}

auto interrupt_line(const char* label, bool active) noexcept
{
    DisabledIf dif = !active;
    ImGui::TextUnformatted(label);
    auto start = ImGui::GetItemRectMin();
    ImVec2 end{start.x + ImGui::GetItemRectSize().x, start.y};
    auto drawlist = ImGui::GetWindowDrawList();
    drawlist->AddLine(start, end, aldo::colors::white(active));
}

auto interrupt_line(const char* label, bool active, aldo_sigstate s) noexcept
{
    interrupt_line(label, active);
    ImGui::SameLine();
    auto pos = ImGui::GetCursorScreenPos();
    ImVec2 center{
        pos.x + 5,
        pos.y + (ImGui::GetTextLineHeight() / 2),
    };
    auto fontSz = ImGui::GetFontSize();
    auto fontColor = s == ALDO_SIG_CLEAR ? IM_COL32_WHITE : IM_COL32_BLACK;
    ImVec2 offset{fontSz / 4, fontSz / 2};
    auto drawList = ImGui::GetWindowDrawList();
    drawList->AddCircleFilled(center, offset.y + 1, signal_color(s));
    drawList->AddText(center - offset, fontColor, signal_label(s));
}

auto small_led(bool on, float xOffset = 0) noexcept
{
    auto pos = ImGui::GetCursorScreenPos();
    ImVec2 center{
        pos.x + xOffset,
        pos.y + (ImGui::GetTextLineHeight() / 2) + 1,
    };
    auto fill = on ? aldo::colors::LedOn : aldo::colors::LedOff;
    auto drawList = ImGui::GetWindowDrawList();
    drawList->AddCircleFilled(center, aldo::style::SmallRadius, fill);
}

//
// MARK: - Concrete Views
//

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
            ImGui::Text("Format: %s", aldo_cart_formatname(info->format));
            ImGui::Separator();
            if (info->format == ALDO_CRTF_INES) {
                renderiNesInfo(info.value());
            } else {
                renderRawInfo();
            }
        } else {
            ImGui::TextUnformatted("Format: None");
        }
    }

private:
    static void renderiNesInfo(const aldo_cartinfo& info) noexcept
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

        ImGui::Text("NT-Mirroring: %s",
                    aldo_ntmirror_name(info.ines_hdr.mirror));
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
        if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderRegisters();
        }
        if (ImGui::CollapsingHeader("Flags", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderFlags();
        }
        if (ImGui::CollapsingHeader("Datapath", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderDatapath();
        }
    }

private:
    void renderRegisters() const
    {
        auto& cpu = emu.snapshot().cpu;
        widget_group([&cpu] noexcept {
            ImGui::Text("A: %02X", cpu.accumulator);
            ImGui::Text("X: %02X", cpu.xindex);
            ImGui::Text("Y: %02X", cpu.yindex);
        });
        ImGui::SameLine(0, 90);
        widget_group([&cpu] noexcept {
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
        // adjust spacer width for SYNC's extra letter
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
            std::array<aldo::et::tchar, AldoDisDatapSize> buf;
            auto err = aldo_dis_datapath(emu.snapshotp(), buf.data());
            ImGui::Text("Decode: %s",
                        err < 0 ? aldo_dis_errstr(err) : buf.data());
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
        auto& datapath = emu.snapshot().cpu.datapath;
        interrupt_line("IRQ", lines.irq, datapath.irq);
        ImGui::SameLine(0, spacer);
        interrupt_line("NMI", lines.nmi, datapath.nmi);
        ImGui::SameLine(0, spacer);
        interrupt_line("RST", lines.reset, datapath.rst);
    }

    static void renderCycleIndicator(int cycle) noexcept
    {
        ImGui::TextUnformatted("t:");
        ImGui::SameLine();
        for (auto i = 0; i < aldo_nes_max_tcpu(); ++i) {
            small_led(i == cycle, aldo::style::SmallRadius * 3 * i);
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
};

class DebuggerView final : public aldo::View {
public:
    DebuggerView(aldo::viewstate& vs, const aldo::Emulator& emu,
                 const aldo::MediaRuntime& mr)
    : View{"Debugger", vs, emu, mr}, conditionsCombo{createCombo()}
    {}
    DebuggerView(aldo::viewstate&, aldo::Emulator&&,
                 const aldo::MediaRuntime&) = delete;
    DebuggerView(aldo::viewstate&, const aldo::Emulator&,
                 aldo::MediaRuntime&&) = delete;
    DebuggerView(aldo::viewstate&, aldo::Emulator&&,
                 aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (ImGui::CollapsingHeader("Reset Vector", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderVectorOverride();
        }

        if (ImGui::CollapsingHeader("Breakpoints", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderBreakpoints();
        }
    }

private:
    using HaltCombo = ComboList<aldo_haltcondition>;
    using bp_sz = aldo::Debugger::BpView::size_type;
    using bp_diff = aldo::Debugger::BreakpointIterator::difference_type;

    static_assert(std::same_as<HaltCombo::option::second_type, aldo::et::str>,
                  "HaltCombo label type does not match emulator string type");

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
                // Add current idx last to mark it as the
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
            // Queue remove commands in descending order to avoid
            // invalidating bp indices during removal.
            decltype(selections) sorted(selections.size());
            std::ranges::partial_sort_copy(selections, sorted,
                                           std::ranges::greater{});
            std::unordered_set<bp_diff> removed(sorted.size());
            for (auto idx : sorted) {
                // here's where duplicate selections bite us
                if (removed.contains(idx)) continue;
                vs.commands.emplace(aldo::Command::breakpointRemove, idx);
                removed.insert(idx);
            }
        }

    private:
        // Arguably this should be a set but the ordered property of
        // vector gives saner multi-select behavior when mixing shift and cmd
        // clicks where the last element can be treated as the most recent
        // selection; duplicates are handled properly with std::erase.
        std::vector<bp_diff> selections;
    };

    HaltCombo createCombo()
    {
        using halt_option = HaltCombo::option;
        using halt_selection = halt_option::first_type;
        using halt_integral = std::underlying_type_t<halt_selection>;

        // does not include first enum value HLT_NONE
        std::array<halt_option, ALDO_HLT_COUNT - 1> conditions;
        std::ranges::generate(conditions,
            [h = static_cast<halt_integral>(ALDO_HLT_NONE)] mutable noexcept
                              -> halt_option {
                auto cond = static_cast<halt_selection>(++h);
                return {cond, aldo_haltcond_description(cond)};
            });

        auto cb = [this](halt_selection v, bool changed) noexcept {
            this->setFocus = true;
            if (changed) {
                this->currentHaltExpression = {.cond = v};
            }
        };

        HaltCombo combo{"##haltconditions", conditions, cb};
        return combo;
    }

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
        DisabledIf dif = [this] noexcept {
            if (this->resetOverride) return false;
            // +2 = start of reset vector
            this->resetAddr = aldo_batowr(this->emu.snapshot().prg.vectors
                                          + 2);
            return true;
        }();
        if (input_address(&resetAddr)) {
            vs.commands.emplace(aldo::Command::resetVectorOverride,
                                static_cast<int>(resetAddr));
        }
    }

    void renderBreakpoints()
    {
        setFocus = false;
        renderConditionCombo();
        ImGui::Separator();
        renderBreakpointAdd();
        ImGui::Separator();
        renderBreakpointList();
        detectedHalt = emu.debugger().halted();
    }

    void renderConditionCombo() noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Halt on");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 12);
        conditionsCombo.render();
    }

    void renderBreakpointAdd()
    {
        switch (currentHaltExpression.cond) {
        case ALDO_HLT_ADDR:
            input_address(&currentHaltExpression.address);
            break;
        case ALDO_HLT_TIME:
            ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 18);
            ImGui::InputFloat("Seconds", &currentHaltExpression.runtime);
            break;
        case ALDO_HLT_CYCLES:
            u64Selection(currentHaltExpression.cycles);
            break;
        case ALDO_HLT_FRAMES:
            u64Selection(currentHaltExpression.frames);
            break;
        case ALDO_HLT_JAM:
            ImGui::Dummy({0, ImGui::GetFrameHeight()});
            break;
        default:
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Invalid halt condition selection: %d",
                        conditionsCombo.selection());
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

    void renderBreakpoint(bp_diff idx, bp_diff breakIdx,
                          const aldo_breakpoint& bp)
    {
        auto bpBreak = idx == breakIdx;
        if (bpBreak && !detectedHalt) {
            bpSelections.select(idx);
        }
        aldo::hexpr_buffer fmt;
        auto err = aldo_haltexpr_desc(&bp.expr, fmt.data());
        ScopedStyleFlt style{
            {ImGuiStyleVar_Alpha, ImGui::GetStyle().DisabledAlpha},
            !bp.enabled,
        };
        ScopedColor color{
            {ImGuiCol_Text, aldo::colors::Attention}, bpBreak,
        };
        ScopedID id = static_cast<int>(idx);
        if (ImGui::Selectable(err < 0 ? aldo_haltexpr_errstr(err) : fmt.data(),
                              bpSelections.selected(idx))) {
            if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
                bpSelections.rangeSelect(idx);
            } else if (ImGui::IsKeyDown(ImGuiMod_Super)) {
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

    void syncWithOverrideState() noexcept
    {
        // If view flag doesn't match debugger state then debugger
        // state was changed under the hood (e.g. by loading a brk file).
        // TODO: if another case like this comes up it may be worth coming up
        // with a model -> view notification system.
        auto& dbg = emu.debugger();
        if ((resetOverride = dbg.isVectorOverridden())) {
            resetAddr = static_cast<aldo::et::word>(dbg.vectorOverride());
        }
    }

    static void u64Selection(aldo::et::qword& input) noexcept
    {
        ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 18);
        ImGui::InputScalar("Count", ImGuiDataType_U64, &input);
    }

    bool detectedHalt = false, resetOverride = false, setFocus = false;
    aldo::et::word resetAddr = 0x0;
    HaltCombo conditionsCombo;
    aldo_haltexpr currentHaltExpression;
    SelectedBreakpoints bpSelections;
};

class NametablesView final : public aldo::View {
public:
    NametablesView(aldo::viewstate& vs, const aldo::Emulator& emu,
                   const aldo::MediaRuntime& mr) noexcept
    : View{"Nametables", vs, emu, mr}, nametables{emu.screenSize(), mr},
    modeCombo{
        "##displayMode",
        {
            {DisplayMode::nametables, "Nametables"},
            {DisplayMode::attributes, "Attributes"},
        },
        [this](DisplayMode v, bool) { this->nametables.mode = v; },
    } {}
    NametablesView(aldo::viewstate&, aldo::Emulator&&,
                   const aldo::MediaRuntime&) = delete;
    NametablesView(aldo::viewstate&, const aldo::Emulator&,
                   aldo::MediaRuntime&&) = delete;
    NametablesView(aldo::viewstate&, aldo::Emulator&&,
                   aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        auto textOffset = nametables.nametableSize().x
                            + aldo::style::glyph_size().x + 1;
        tableLabel(0);
        ImGui::SameLine(textOffset);
        tableLabel(1);

        renderNametables();

        tableLabel(2);
        ImGui::SameLine(textOffset);
        tableLabel(3);
        ImGui::Text("Mirroring: %s",
                    aldo_ntmirror_name(emu.snapshot().video->nt.mirror));

        ImGui::Separator();

        renderControls();
    }

private:
    class ALDO_SIDEFX NtOverlays {
    public:
        NtOverlays(ImDrawListSplitter& s, const aldo::Nametables& nt,
                   const aldo_snapshot& snp, bool tg, bool ag,
                   bool si) noexcept
        : splitter{s}, drawList{ImGui::GetWindowDrawList()},
        ntOrigin{ImGui::GetCursorScreenPos()},
        ntExtent{point_to_vec(nt.nametableExtent())},
        scrSize{point_to_vec(nt.nametableSize())}, attrGrid{ag}, scrInd{si},
        tileGrid{tg}
        {
            const auto& emuPos = snp.video->nt.pos;
            scrPos = {
                emuPos.x + (emuPos.h * scrSize.x),
                emuPos.y + (emuPos.v * scrSize.y),
            };
            splitter.Split(drawList, 2);
        }
        NtOverlays(const NtOverlays&) = delete;
        NtOverlays& operator=(const NtOverlays&) = delete;
        NtOverlays(NtOverlays&&) = delete;
        NtOverlays& operator=(NtOverlays&&) = delete;
        ~NtOverlays() { splitter.Merge(drawList); }

        void render() const
        {
            splitter.SetCurrentChannel(drawList, 1);
            if (tileGrid) {
                renderTileGrid();
            }
            if (attrGrid) {
                renderAttributeGrid();
            }
            if (scrInd) {
                renderScreenIndicator();
            }
        }

        void render(const auto& interval, SDL_Point& refreshPos,
                    const aldo::viewstate& vs)
        {
            if (!scrInd) return;
            if (interval.elapsed(vs.clock.clock())) {
                ++refreshPos.x %= 512;
                ++refreshPos.y %= 480;
                //if (--refreshPos.x < 0) {
                //    refreshPos.x = 511;
                //}
                //if (--refreshPos.y < 0) {
                //    refreshPos.y = 479;
                //}
            }
            scrPos = point_to_vec(refreshPos);
            render();
        }

    private:
        class ALDO_SIDEFX BackChannel {
        public:
            BackChannel(ImDrawListSplitter& s, ImDrawList* dl) noexcept
            : splitter{s}, drawList{dl}
            { splitter.SetCurrentChannel(drawList, 0); }
            BackChannel(const BackChannel&) = delete;
            BackChannel& operator=(const BackChannel&) = delete;
            BackChannel(BackChannel&&) = delete;
            BackChannel& operator=(BackChannel&&) = delete;
            ~BackChannel() { splitter.SetCurrentChannel(drawList, 1); }

        private:
            ImDrawListSplitter& splitter;
            ImDrawList* drawList;
        };

        void renderTileGrid() const
        {
            static constexpr auto step = decltype(nametables)::TilePxDim;

            auto [max, hor, ver] = gridDims();
            for (auto start = ntOrigin;
                 start.x - ntOrigin.x < max.x;
                 start.x += step) {
                drawList->AddLine(start, start + ver, aldo::colors::Attention);
            }
            for (auto start = ntOrigin;
                 start.y - ntOrigin.y < max.y;
                 start.y += step) {
                drawList->AddLine(start, start + hor, aldo::colors::Attention);
            }
        }

        void renderAttributeGrid() const
        {
            static constexpr auto step = decltype(nametables)::AttributePxDim;

            auto [max, hor, ver] = gridDims();
            for (auto start = ntOrigin;
                 start.x - ntOrigin.x < max.x;
                 start.x += step) {
                drawList->AddLine(start, start + ver, aldo::colors::LedOn);
            }
            for (auto start = ntOrigin;
                 start.y - ntOrigin.y < scrSize.y;
                 start.y += step) {
                drawList->AddLine(start, start + hor, aldo::colors::LedOn);
            }
            for (ImVec2 start{ntOrigin.x, ntOrigin.y + scrSize.y};
                 start.y - ntOrigin.y < max.y;
                 start.y += step) {
                drawList->AddLine(start, start + hor, aldo::colors::LedOn);
            }
            ImVec2 lastLine{ntOrigin.x, ntOrigin.y + ntExtent.y};
            drawList->AddLine(lastLine, lastLine + hor, aldo::colors::LedOn);
        }

        void renderScreenIndicator() const noexcept
        {
            ClipRect clip{ntOrigin, ntExtent};
            renderScreenHighlight();
            renderScreenPosition();
        }

        void renderScreenHighlight() const noexcept
        {
            BackChannel useBackChannel{splitter, drawList};

            // Top Left
            auto start = scrPos, end = scrPos;
            start.x -= scrSize.x;
            start.y -= ntExtent.y;
            end.y -= scrSize.y;
            drawScreenOverlay(start, end);

            // Mid Left
            start = end = scrPos;
            start.x -= scrSize.x;
            end.y += scrSize.y;
            drawScreenOverlay(start, end);

            // Upper Row
            start = end = scrPos;
            start.x -= ntExtent.x;
            start.y -= scrSize.y;
            end.x += scrSize.x;
            drawScreenOverlay(start, end);

            // Bottom Row
            start = end = scrPos;
            start.x -= ntExtent.x;
            start.y += scrSize.y;
            end.x += scrSize.x;
            end.y += ntExtent.y;
            drawScreenOverlay(start, end);

            // Right Column
            start = end = scrPos;
            start.x += scrSize.x;
            start.y -= ntExtent.y;
            end = end + ntExtent;
            drawScreenOverlay(start, end);
        }

        void renderScreenPosition() const noexcept
        {
            drawScreenIndicator(scrPos);

            bool hClipped = clipped(scrPos.x, scrSize.x);
            if (hClipped) {
                auto hOverflow = scrPos;
                hOverflow.x -= ntExtent.x;
                drawScreenIndicator(hOverflow);
            }

            bool vClipped = clipped(scrPos.y, scrSize.y);
            if (vClipped) {
                auto vOverflow = scrPos;
                vOverflow.y -= ntExtent.y;
                drawScreenIndicator(vOverflow);
            }

            if (hClipped && vClipped) {
                drawScreenIndicator(scrPos - ntExtent);
            }
        }

        void drawScreenIndicator(const ImVec2& start) const noexcept
        {
            auto orig = ntOrigin + start;
            drawList->AddRect(orig, orig + scrSize, aldo::colors::LineIn, 0.0f,
                              ImDrawFlags_None, 2.0f);
        }

        void drawScreenOverlay(const ImVec2& start,
                               const ImVec2& end) const noexcept
        {
            drawList->AddRectFilled(ntOrigin + start, ntOrigin + end,
                                    aldo::colors::DarkOverlay);
        }

        std::tuple<ImVec2, ImVec2, ImVec2> gridDims() const
        {
            return {ntExtent + 1, {ntExtent.x, 0}, {0, ntExtent.y}};
        }

        static bool clipped(float offset, float size) noexcept
        {
            return offset - size > 0;
        }

        ImDrawListSplitter& splitter;
        ImDrawList* drawList;
        ImVec2 ntOrigin, ntExtent, scrPos, scrSize;
        bool attrGrid, scrInd, tileGrid;
    };

    void renderNametables()
    {
        // TODO: include tooltip hover for tiles
        NtOverlays ov{
            splitter, nametables, emu.snapshot(), tileGrid, attributeGrid,
            screenIndicator,
        };
        if (drawInterval.elapsed(vs.clock.clock())) {
            nametables.draw(emu, mr);
        }
        nametables.render();
        ov.render();
        //ov.render(screenInterval, screenPos, vs);
    }

    void renderControls() noexcept
    {
        ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 15);
        modeCombo.render();
        ImGui::SameLine();
        widget_group([this] noexcept {
            ImGui::Checkbox("Tile Grid", &this->tileGrid);
            ImGui::Checkbox("Attribute Grid", &this->attributeGrid);
        });
        ImGui::SameLine();
        ImGui::Checkbox("Screen Indicator", &screenIndicator);
    }

    void tableLabel(int table) const noexcept
    {
        auto attr = modeCombo.selection() == DisplayMode::attributes;
        ImGui::Text("%s $%4X", attr ? "Attribute Table" : "Nametable",
                    ALDO_MEMBLOCK_8KB + (table * ALDO_MEMBLOCK_1KB)
                    + (attr * AldoNtTileCount));
    }

    aldo::Nametables nametables;
    ImDrawListSplitter splitter;
    using DisplayMode = decltype(nametables)::DrawMode;
    ComboList<DisplayMode> modeCombo;
    RefreshInterval<250.0> drawInterval;
    bool attributeGrid = false, screenIndicator = false, tileGrid = false;

    [[maybe_unused]]
    RefreshInterval<50.0> screenInterval;
    [[maybe_unused]]
    SDL_Point screenPos{};
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
    using pal_c = aldo::palette::datav;

    static void renderHeader() noexcept
    {
        std::array<char, 3> buf;
        ImGui::TableSetupColumn("Idx", ImGuiTableColumnFlags_WidthStretch);
        for (pal_sz col = 1; col < Cols; ++col) {
            std::snprintf(buf.data(), buf.size(), " %zX", col - 1);
            ImGui::TableSetupColumn(buf.data());
        }
        ImGui::TableHeadersRow();
    }

    void renderBody()
    {
        static constexpr pal_sz rows = 4, paletteDim = aldo::palette::Size / rows;

        ScopedStyleVec textAlign{{
            ImGuiStyleVar_SelectableTextAlign,
            aldo::style::AlignCenter,
        }};
        for (pal_sz row = 0; row < rows; ++row) {
            for (pal_sz col = 0; col < Cols; ++col) {
                auto rowStart = paletteDim * row;
                ImGui::TableNextColumn();
                if (col == 0) {
                    ImGui::Text(" %02zX", rowStart);
                } else {
                    auto cell = rowStart + (col - 1);
                    assert(cell < aldo::palette::Size);
                    auto color = lookupColor(cell);
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
                    ScopedColor indicatorColor{
                        {ImGuiCol_Text, text_contrast(color)},
                        cell == vs.colorSelection,
                    };
                    ScopedID id = static_cast<int>(cell);
                    if (ImGui::Selectable(cell == vs.colorSelection ? "x" : "",
                                          false, ImGuiSelectableFlags_None,
                                          aldo::style::CellSz)) {
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
        widget_group([this] noexcept {
            bool chr = ImGui::Checkbox("Red Emphasis", &this->emr);
            bool chg = ImGui::Checkbox("Green Emphasis", &this->emg);
            bool chb = ImGui::Checkbox("Blue Emphasis", &this->emb);
            if (chr || chg || chb) {
                this->updateEmphasis();
            }
        });
        ImGui::SameLine();
        ImGui::Checkbox("Grayscale", &gray);
    }

    void renderColorSelection() const
    {
        static constexpr auto selectedColorDim = 70;
        auto color = lookupColor(vs.colorSelection);
        ImGui::ColorButton("Selected Color",
                           ImGui::ColorConvertU32ToFloat4(color),
                           ImGuiColorEditFlags_NoAlpha
                           | ImGuiColorEditFlags_NoTooltip
                           | ImGuiColorEditFlags_NoDragDrop,
                           {selectedColorDim, selectedColorDim});
        ImGui::SameLine();
        widget_group([color, &vs = vs] {
            auto [r, g, b] = aldo::colors::rgb(color);
            ImGui::Text("Index: %02zX", vs.colorSelection);
            ImGui::Text("RGBd:  %d %d %d", r, g, b);
            ImGui::Text("RGBx:  %02X %02X %02X", r, g, b);
            ImGui::Text("Hex:   #%02x%02x%02x", r, g, b);
            if (vs.colorSelection == 0xd) {
                ImGui::TextUnformatted("(forbidden black)");
            }
        });
    }

    pal_c lookupColor(pal_sz idx) const
    {
        auto color = emu.palette().getColor(adjustIdx(idx));
        return applyEmphasis(color, idx);
    }

    pal_sz adjustIdx(pal_sz idx) const noexcept
    {
        return gray ? idx & 0x30 : idx;
    }

    pal_c applyEmphasis(pal_c color, pal_sz idx) const
    {
        // Minimum channel value used for applying emphasis in order to
        // get a visible tint on colors that would otherwise round down to
        // nearly zero.
        static constexpr auto emFloor = 0x10;

        if ((emr || emg || emb) && ((idx & 0xe) < 0xe)) {
            auto [r, g, b] = aldo::colors::rgb_floor(color, emFloor);
            color = IM_COL32(static_cast<float>(r) * atr,
                             static_cast<float>(g) * atg,
                             static_cast<float>(b) * atb,
                             SDL_ALPHA_OPAQUE);
        }
        return color;
    }

    void updateEmphasis() noexcept
    {
        // Think of this as a bitmask of emphasis BGR where emphasis set
        // means no attenuation and emphasis clear means attenuation except
        // 000 and 111 mean the opposite.
        if (emr && emg && emb) {
            // all emphasis attenuates all colors
            atr = atg = atb = Attenuated;
        } else if (!emr && !emg && !emb) {
            // no emphasis attenuates no colors
            atr = atg = atb = Full;
        } else {
            // otherwise emphasis applies no attenuation and vice-versa
            atr = emr ? Full : Attenuated;
            atg = emg ? Full : Attenuated;
            atb = emb ? Full : Attenuated;
        }
    }

    static constexpr pal_sz Cols = 17;
    static constexpr float Attenuated = 0.816328f, Full = 1.0f;

    bool gray = false, emr = false, emg = false, emb = false;
    float atr = Full, atg = Full, atb = Full;
};

class PatternTablesView final : public aldo::View {
public:
    PatternTablesView(aldo::viewstate& vs, const aldo::Emulator& emu,
                      const aldo::MediaRuntime& mr)
    : View{"Pattern Tables", vs, emu, mr}, left{mr}, right{mr} {}
    PatternTablesView(aldo::viewstate&, aldo::Emulator&&,
                      const aldo::MediaRuntime&) = delete;
    PatternTablesView(aldo::viewstate&, const aldo::Emulator&,
                      aldo::MediaRuntime&&) = delete;
    PatternTablesView(aldo::viewstate&, aldo::Emulator&&,
                      aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (drawInterval.elapsed(vs.clock.clock())) {
            auto vsp = emu.snapshot().video;
            const auto& tables = vsp->pattern_tables;
            assert(palSelect < PalSize * 2);
            colspan colors = palSelect < PalSize
                                ? vsp->palettes.bg[palSelect]
                                : vsp->palettes.fg[palSelect - PalSize];
            left.draw(tables.left, colors, emu.palette());
            right.draw(tables.right, colors, emu.palette());
        }

        widget_group([this] noexcept {
            ImGui::TextUnformatted("Pattern Table $0000");
            this->left.render();
        });
        ImGui::SameLine(0, 10);
        widget_group([this] noexcept {
            ImGui::TextUnformatted("Pattern Table $1000");
            this->right.render();
        });
        ImGui::SameLine(0, 10);
        widget_group([this] {
            this->renderPalettes(false);
            this->renderPalettes(true);
            this->renderEmphasis();
        });
    }

private:
    using colspan = aldo::color_span;

    void renderPalettes(bool fg)
    {
        static constexpr auto cols = static_cast<int>(PalSize + 1);
        static constexpr auto flags = ImGuiTableFlags_BordersInner
                                        | ImGuiTableFlags_SizingFixedFit;

        ImGui::TextUnformatted(fg ? "Sprites" : "Background");
        std::span pals = fg
                            ? emu.snapshot().video->palettes.fg
                            : emu.snapshot().video->palettes.bg;
        auto row = 0;
        for (colspan pal : pals) {
            if (ImGui::BeginTable(fg ? "FgPalettes" : "BgPalettes", cols, flags)) {
                ScopedStyleVec textAlign{{
                    ImGuiStyleVar_SelectableTextAlign,
                    aldo::style::AlignCenter,
                }};
                std::array<char, 3> buf;
                for (auto col = 0; col < cols; ++col) {
                    ImGui::TableNextColumn();
                    if (col == 0) {
                        std::snprintf(buf.data(), buf.size(), "%d",
                                      fg ? row + 4 : row);
                        auto palIdx = static_cast<aldo::et::size>(row + (fg << 2));
                        if (ImGui::Selectable(buf.data(), palSelect == palIdx,
                                              ImGuiSelectableFlags_None,
                                              aldo::style::CellSz)) {
                            palSelect = palIdx;
                        }
                    } else {
                        auto idx = pal[static_cast<decltype(pal)::size_type>(col - 1)];
                        auto color = emu.palette().getColor(idx);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
                        std::snprintf(buf.data(), buf.size(), "%02X", idx);
                        ScopedColor txtColor{{
                            ImGuiCol_Text,
                            text_contrast(color),
                        }};
                        ScopedID id = row << 3 | col << 1 | fg;
                        if (ImGui::Selectable(buf.data(), false,
                                              ImGuiSelectableFlags_None,
                                              aldo::style::CellSz)) {
                            vs.colorSelection = idx;
                        }
                    }
                }
                ImGui::EndTable();
            }
            ++row;
        }
    }

    void renderEmphasis() const noexcept
    {
        auto spacer = aldo::style::glyph_size().x * 3.5f;

        ImGui::TextUnformatted("Emphasis");

        auto mask = emu.snapshot().ppu.mask;
        ImGui::TextUnformatted("Red: ");
        ImGui::SameLine();
        small_led(mask & 0x20);

        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted("Green:");
        ImGui::SameLine();
        small_led(mask & 0x40);

        ImGui::Spacing();

        ImGui::TextUnformatted("Blue:");
        ImGui::SameLine();
        small_led(mask & 0x80);

        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted("Gray: ");
        ImGui::SameLine();
        small_led(mask & 0x1);
    }

    static constexpr colspan::size_type PalSize = colspan::extent;

    aldo::PatternTable left, right;
    RefreshInterval<500.0> drawInterval;
    aldo::et::size palSelect = 0;
};

class PpuView final : public aldo::View {
public:
    PpuView(aldo::viewstate& vs, const aldo::Emulator& emu,
            const aldo::MediaRuntime& mr) noexcept
    : View{"PPU", vs, emu, mr} {}
    PpuView(aldo::viewstate&, aldo::Emulator&&,
            const aldo::MediaRuntime&) = delete;
    PpuView(aldo::viewstate&, const aldo::Emulator&,
            aldo::MediaRuntime&&) = delete;
    PpuView(aldo::viewstate&, aldo::Emulator&&, aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        if (ImGui::CollapsingHeader("Registers", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderRegisters();
        }
        if (ImGui::CollapsingHeader("Pipeline", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderPipeline();
        }
    }

private:
    void renderRegisters() const
    {
        auto& ppu = emu.snapshot().ppu;
        widget_group([&ppu] noexcept {
            ImGui::Text("CTRL: %02X", ppu.ctrl);
            ImGui::Text("MASK: %02X", ppu.mask);
        });
        ImGui::SameLine(0, 20);
        widget_group([&ppu] noexcept {
            ImGui::Text("STATUS:  %d%d%d", aldo_getbit(ppu.status, 7),
                        aldo_getbit(ppu.status, 6),
                        aldo_getbit(ppu.status, 5));
            ImGui::Text("OAMADDR: %02X", ppu.oamaddr);
        });
    }

    void renderPipeline() const noexcept
    {
        renderDataLines();
        ImGui::Separator();
        renderSignals();
        ImGui::Separator();
        renderPixelProcessing();
    }

    void renderDataLines() const noexcept
    {
        auto& pipeline = emu.snapshot().ppu.pipeline;
        auto& lines = emu.snapshot().ppu.lines;

        {
            ScopedColor color{{ImGuiCol_Text, aldo::colors::LineIn}};
            ImGui::Text("RgSel: %d%d%d",
                        aldo_getbit(pipeline.register_select, 2),
                        aldo_getbit(pipeline.register_select, 1),
                        aldo_getbit(pipeline.register_select, 0));
        }
        ImGui::SameLine(0, 27);
        {
            ScopedColor color{{
                ImGuiCol_Text,
                lines.cpu_readwrite
                    ? aldo::colors::LineOut
                    : aldo::colors::LineIn,
            }};
            ImGui::Text("RgVal: %02X", pipeline.register_databus);
        }
        {
            DisabledIf dif = !lines.address_enable;
            ScopedColor color{{ImGuiCol_Text, aldo::colors::LineOut}};
            ImGui::Text("VAddr: %04X", pipeline.addressbus);
        }
        ImGui::SameLine(0, 20);
        {
            DisabledIf dif = lines.read && lines.write;
            ScopedColor color{{
                ImGuiCol_Text,
                lines.read ? aldo::colors::LineOut : aldo::colors::LineIn,
            }};
            ImGui::Text("VData: %02X", pipeline.databus);
        }
    }

    void renderSignals() const noexcept
    {
        auto& pipeline = emu.snapshot().ppu.pipeline;
        auto& lines = emu.snapshot().ppu.lines;

        ImGui::Spacing();
        interrupt_line("RST", lines.reset, pipeline.rst);
        ImGui::SameLine(0, 116);
        interrupt_line("INT", lines.interrupt);
    }

    void renderPixelProcessing() const noexcept
    {
        auto& pipeline = emu.snapshot().ppu.pipeline;
        auto& lines = emu.snapshot().ppu.lines;

        renderScrollAddr('v', pipeline.scrolladdr);
        renderScrollAddr('t', pipeline.tempaddr);
        ImGui::Text("x: %02X (%d)", pipeline.xfine, pipeline.xfine);
        ImGui::SameLine(0, 28);
        ImGui::Text("r: %02X", pipeline.readbuffer);
        {
            DisabledIf dif = !lines.video_out;
            ImGui::Text("(%3d,%3d): %02X", pipeline.line, pipeline.dot,
                        pipeline.pixel);
        }
        auto spacer = aldo::style::glyph_size().x * 3.5f;
        ImGui::TextUnformatted("c:");
        ImGui::SameLine();
        small_led(pipeline.cv_pending);
        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted("o:");
        ImGui::SameLine();
        small_led(pipeline.oddframe);
        ImGui::SameLine(0, spacer);
        ImGui::TextUnformatted("w:");
        ImGui::SameLine();
        small_led(pipeline.writelatch);
    }

    static void renderScrollAddr(char label, aldo::et::word addr) noexcept
    {
        ImGui::Text("%c: %04X (%d,%d,%02d,%02d)", label, addr,
                    (addr & 0x7000) >> 12, (addr & 0xc00) >> 10,
                    (addr & 0x3e0) >> 5, addr & 0x1f);
    }
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
        if (ImGui::CollapsingHeader("Vectors", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderVectors();
        }
    }

private:
    void renderPrg() noexcept
    {
        static constexpr auto instCount = 16;

        auto curr = emu.snapshot().prg.curr;
        auto addr = emu.snapshot().cpu.datapath.current_instruction;
        aldo_dis_instruction inst{};
        std::array<aldo::et::tchar, AldoDisInstSize> disasm;
        for (auto i = 0; i < instCount; ++i) {
            auto result = aldo_dis_parsemem_inst(curr->length, curr->pc,
                                                 inst.offset + inst.bv.size,
                                                 &inst);
            if (result > 0) {
                result = aldo_dis_inst(addr, &inst, disasm.data());
                if (result > 0) {
                    if (ImGui::Selectable(disasm.data(), selected == i)) {
                        selected = i;
                    } else if (ImGui::BeginPopupContextItem()) {
                        selected = i;
                        if (ImGui::Selectable("Add breakpoint...")) {
                            aldo_haltexpr expr{
                                .address = addr, .cond = ALDO_HLT_ADDR,
                            };
                            vs.commands.emplace(aldo::Command::breakpointAdd, expr);
                        }
                        ImGui::EndPopup();
                    }
                    addr += static_cast<aldo::et::word>(inst.bv.size);
                    continue;
                }
            }
            if (result < 0) {
                ImGui::TextUnformatted(aldo_dis_errstr(result));
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
        ImGui::Text("%4X: %02X %02X     NMI $%04X", ALDO_CPU_VECTOR_NMI, lo,
                    hi, aldo_bytowr(lo, hi));

        lo = prg.vectors[2];
        hi = prg.vectors[3];
        const char* indicator;
        aldo::et::word resVector;
        auto& dbg = emu.debugger();
        if (dbg.isVectorOverridden()) {
            indicator = ALDO_HEXPR_RST_IND;
            resVector = static_cast<aldo::et::word>(dbg.vectorOverride());
        } else {
            indicator = "";
            resVector = aldo_bytowr(lo, hi);
        }
        ImGui::Text("%4X: %02X %02X     RST %s$%04X", ALDO_CPU_VECTOR_RST, lo,
                    hi, indicator, resVector);

        lo = prg.vectors[4];
        hi = prg.vectors[5];
        ImGui::Text("%4X: %02X %02X     IRQ $%04X", ALDO_CPU_VECTOR_IRQ, lo,
                    hi, aldo_bytowr(lo, hi));
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
            // account for up to 2 page-break rows per screen
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
            std::snprintf(col.data(), col.size(), " %X", i);
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
                // Since the clipper has some extra padding for the
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
                ScopedColor highlight{{ImGuiCol_Text, IM_COL32_BLACK}};
                ImGui::Text("%02X", val);
            } else {
                ImGui::Text("%02X", val);
            }
            if (std::isprint(static_cast<char>(val), lcl)) {
                ascii[static_cast<ascii_sz>(ramCol)] = static_cast<ascii_val>(val);
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
    };

    static constexpr int
        PageDim = 16, PageSize = PageDim * PageDim, Cols = PageDim + 2;
};

class SpritesView final : public aldo::View {
public:
    SpritesView(aldo::viewstate& vs, const aldo::Emulator& emu,
                const aldo::MediaRuntime& mr) noexcept
    : View{"Sprites", vs, emu, mr}, sprites{mr},
    priorityCombo {
        "Priority",
        {
            {PriorityMode::all, "All"},
            {PriorityMode::front, "Front Only"},
            {PriorityMode::back, "Back Only"},
        },
        [this](PriorityMode v, bool) { this->sprites.priority = v; },
    } {}
    SpritesView(aldo::viewstate&, aldo::Emulator&&,
                const aldo::MediaRuntime&) = delete;
    SpritesView(aldo::viewstate&, const aldo::Emulator&,
                aldo::MediaRuntime&&) = delete;
    SpritesView(aldo::viewstate&, aldo::Emulator&&,
                aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        auto obj = selectedSprite();
        renderSpriteSpace(obj);
        ImGui::SameLine();
        widget_group([this, obj] noexcept {
            this->renderSpriteSelect();
            this->renderSpriteDetails(obj);
        });
        renderSpriteControls();
    }

private:
    class SpriteOverlay {
    public:
        SpriteOverlay(const SDL_Point& s, const SDL_Point& se,
                      const SDL_Point& sd, bool si) noexcept
        : drawList{ImGui::GetWindowDrawList()}, origin{ImGui::GetCursorScreenPos()},
        scrSize{point_to_vec(s)}, sprExtent{point_to_vec(se)},
        overlayDim{point_to_vec(sd)}, screenIndicator{si} {}

        const ImVec2& spriteSize() const noexcept { return sprExtent; }

        std::optional<SDL_FPoint> getMouseClick() const noexcept
        {
            if (ImGui::IsMousePosValid()) {
                auto r = vecs_to_rectf(origin, overlayDim);
                auto p = vec_to_pointf(ImGui::GetMousePos());
                if (SDL_PointInRectFloat(&p, &r)
                    && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    return p - origin;
                }
            }
            return {};
        }

        void render(const aldo::sprite_obj* obj) const noexcept
        {
            drawScreenIndicator();
            drawSpriteSelection(obj);
        }

    private:
        void drawScreenIndicator() const noexcept
        {
            if (!screenIndicator) return;

            drawList->AddRect(origin, origin + scrSize, aldo::colors::LineIn,
                              0.0f, ImDrawFlags_None, 2.0f);
        }

        void drawSpriteSelection(const aldo::sprite_obj* obj) const noexcept
        {
            if (!obj) return;

            ClipRect cr{origin, overlayDim};
            auto spriteOrigin = origin + ImVec2{
                static_cast<float>(obj->x), static_cast<float>(obj->y),
            };
            drawList->AddRect(spriteOrigin, spriteOrigin + sprExtent,
                              aldo::colors::LedOn);
        }

        ImDrawList* drawList;
        ImVec2 origin, scrSize, sprExtent, overlayDim;
        bool screenIndicator;
    };

    void renderSpriteSpace(const aldo::sprite_obj* obj)
    {
        SDL_Point
            sprExtent{sprites.SpritePxDim, sprites.SpritePxDim},
            ovDim{sprites.SpritesDim, sprites.SpritesDim};
        if (emu.snapshot().video->sprites.double_height) {
            sprExtent.y *= 2;
        }
        SpriteOverlay ov{emu.screenSize(), sprExtent, ovDim, screenIndicator};
        handleOverlayClick(ov);
        renderSpriteScreen();
        ov.render(obj);
    }

    void renderSpriteScreen()
    {
        if (drawInterval.elapsed(vs.clock.clock())) {
            sprites.draw(emu);
        }
        sprites.render();
    }

    void renderSpriteSelect() noexcept
    {
        static constexpr auto tblDim = AldoSpriteCount / 8;
        static constexpr auto flags = ImGuiTableFlags_Borders
                                        | ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("Select", tblDim, flags)) {
            std::array<char, 3> buf;
            ScopedStyleVec textAlign{{
                ImGuiStyleVar_SelectableTextAlign,
                aldo::style::AlignCenter,
            }};
            for (auto r = 0; r < tblDim; ++r) {
                for (auto c = 0; c < tblDim; ++c) {
                    auto idx = c + (r * tblDim);
                    ImGui::TableNextColumn();
                    std::snprintf(buf.data(), buf.size(), "%02d", idx);
                    if (ImGui::Selectable(buf.data(), this->selected == idx,
                                          ImGuiSelectableFlags_None,
                                          aldo::style::CellSz)) {
                        selected = idx;
                    }
                }
            }
            ImGui::EndTable();
        }
    }

    void renderSpriteDetails(const aldo::sprite_obj* obj) const noexcept
    {
        if (!obj) return;

        ImGui::Text("Position: (%03d, %03d)", obj->x, obj->y);
        std::array<char, 6> buf;
        if (emu.snapshot().video->sprites.double_height) {
            std::snprintf(buf.data(), buf.size(), "%02X+%02X", obj->tiles[0],
                          obj->tiles[1]);
        } else {
            std::snprintf(buf.data(), buf.size(), "%02X", obj->tiles[0]);
        }
        ImGui::Text("Tile: %s ($%1d000)", buf.data(), obj->pt);

        ImGui::Text("Palette: %d", obj->palette);
        ImGui::Text("Priority: %s", obj->priority ? "Back" : "Front");

        ImGui::Text("H-Flip: %s", boolstr(obj->hflip));
        ImGui::Text("V-Flip: %s", boolstr(obj->vflip));
    }

    void renderSpriteControls() noexcept
    {
        ImGui::Checkbox("Screen Indicator", &screenIndicator);
        auto doubleHeight = emu.snapshot().video->sprites.double_height;
        ImGui::SameLine(0, doubleHeight ? 52 : 59);
        ImGui::Text("Size: 8x%d", doubleHeight ? 16 : 8);
        ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 14);
        priorityCombo.render();
    }

    void handleOverlayClick(const SpriteOverlay& ov) noexcept
    {
        auto mouse = ov.getMouseClick();
        // check for mouse click and priority dropdown not overlapping
        if (!mouse || ImGui::IsAnyItemHovered()) return;

        selected = NoSelection;
        aldo::sprite_span objs = emu.snapshot().video->sprites.objects;
        // TODO: add std::views::enumerate when this exists
        auto idx = static_cast<int>(objs.size() - 1);
        // sprites are drawn first-to-last, so z-order for mouse hit is in reverse
        for (const auto& obj : objs | std::views::reverse) {
            SDL_FRect hit{
                static_cast<float>(obj.x), static_cast<float>(obj.y),
                ov.spriteSize().x, ov.spriteSize().y,
            };
            if (SDL_PointInRectFloat(&mouse.value(), &hit)) {
                selected = idx;
                return;
            }
            --idx;
        }
        assert(idx == NoSelection);
    }

    const aldo::sprite_obj* selectedSprite() const noexcept
    {
        if (selected == NoSelection) return nullptr;

        assert(0 <= selected && static_cast<aldo::et::size>(selected)
               < aldo_arrsz(emu.snapshot().video->sprites.objects));

        return emu.snapshot().video->sprites.objects + selected;
    }

    aldo::Sprites sprites;
    using PriorityMode = decltype(sprites)::Priority;
    ComboList<PriorityMode> priorityCombo;
    RefreshInterval<250.0> drawInterval;
    int selected = NoSelection;
    bool screenIndicator = false;
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
        const auto& clock = vs.clock.clock();
        if (statsInterval.elapsed(clock)) {
            dispDtInput = vs.clock.dtInputMs();
            dispDtUpdate = vs.clock.dtUpdateMs();
            dispDtRender = vs.clock.dtRenderMs();
            dispDtElapsed = vs.clock.dtTotalMs();
            dispDtTick = clock.ticktime_ms;
            dispDtLeft = vs.clock.tickLeft();
        }
        ImGui::Text("Input dT: %.3f", dispDtInput);
        ImGui::Text("Update dT: %.3f", dispDtUpdate);
        ImGui::Text("Render dT: %.3f", dispDtRender);
        ImGui::Text("Elapsed dT: %.3f", dispDtElapsed);
        {
            ScopedColor warn{
                {ImGuiCol_Text, aldo::colors::Attention},
                dispDtLeft < 0,
            };
            ImGui::Text("Tick dT: %.3f (%+.3f)", dispDtTick, dispDtLeft);
        }
        ImGui::Text("Ticks: %" PRIu64, clock.ticks);
        ImGui::Text("Emutime: %.3f", clock.emutime);
        ImGui::Text("Runtime: %.3f", clock.runtime);
        ImGui::Text("Cycles: %" PRIu64, clock.cycles);
        ImGui::Text("Frames: %" PRIu64, clock.frames);
        ImGui::Text("Missed: %" PRIu64, vs.clock.missedTicks());
        ImGui::Text("BCD Support: %s", boolstr(emu.bcdSupport()));
    }

    void renderSpeedControls() const noexcept
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(vs.clock.scale() == ALDO_CS_CYCLE
                               ? "Cycles/Second"
                               : "Frames/Second");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 6);
        int min, max;
        if (vs.clock.scale() == ALDO_CS_CYCLE) {
            min = Aldo_MinCps;
            max = Aldo_MaxCps;
        } else {
            min = Aldo_MinFps;
            max = Aldo_MaxFps;
        }
        ImGui::DragInt("##clockRate", &vs.clock.clock().rate, 1, min, max,
                       "%d", ImGuiSliderFlags_AlwaysClamp);

        if (ImGui::RadioButton("Cycles", vs.clock.scale() == ALDO_CS_CYCLE)) {
            vs.clock.setScale(ALDO_CS_CYCLE);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Frames", vs.clock.scale() == ALDO_CS_FRAME)) {
            vs.clock.setScale(ALDO_CS_FRAME);
        }
    }

    void renderRunControls() const
    {
        auto& lines = emu.snapshot().cpu.lines;
        auto halt = !lines.ready;
        if (ImGui::Checkbox("HALT", &halt)) {
            vs.commands.emplace(aldo::Command::ready, !halt);
        };

        auto mode = emu.runMode();
        if (ImGui::RadioButton("Sub ", mode == ALDO_EXC_SUBCYCLE)
            && mode != ALDO_EXC_SUBCYCLE) {
            vs.commands.emplace(aldo::Command::mode, ALDO_EXC_SUBCYCLE);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Cycle", mode == ALDO_EXC_CYCLE)
            && mode != ALDO_EXC_CYCLE) {
            vs.commands.emplace(aldo::Command::mode, ALDO_EXC_CYCLE);
        }
        if (ImGui::RadioButton("Step", mode == ALDO_EXC_STEP)
            && mode != ALDO_EXC_STEP) {
            vs.commands.emplace(aldo::Command::mode, ALDO_EXC_STEP);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Run", mode == ALDO_EXC_RUN)
            && mode != ALDO_EXC_RUN) {
            vs.commands.emplace(aldo::Command::mode, ALDO_EXC_RUN);
        }

        auto
            irq = emu.probe(ALDO_INT_IRQ),
            nmi = emu.probe(ALDO_INT_NMI),
            rst = emu.probe(ALDO_INT_RST);
        if (ImGui::Checkbox("IRQ", &irq)) {
            vs.addProbeCommand(ALDO_INT_IRQ, irq);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("NMI", &nmi)) {
            vs.addProbeCommand(ALDO_INT_NMI, nmi);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("RST", &rst)) {
            vs.addProbeCommand(ALDO_INT_RST, rst);
        }
    }

    RefreshInterval<200.0> statsInterval;
    double
        dispDtInput = 0, dispDtUpdate = 0, dispDtRender = 0, dispDtElapsed = 0,
        dispDtTick = 0, dispDtLeft = 0;
};

class VideoView final : public aldo::View {
public:
    VideoView(aldo::viewstate& vs, const aldo::Emulator& emu,
              const aldo::MediaRuntime& mr)
    : View{"Video", vs, emu, mr, ImGuiWindowFlags_AlwaysAutoResize},
        screen{emu.screenSize(), mr} {}
    VideoView(aldo::viewstate&, aldo::Emulator&&,
              const aldo::MediaRuntime&) = delete;
    VideoView(aldo::viewstate&, const aldo::Emulator&,
              aldo::MediaRuntime&&) = delete;
    VideoView(aldo::viewstate&, aldo::Emulator&&,
              aldo::MediaRuntime&&) = delete;

protected:
    void renderContents() override
    {
        static constexpr std::array scales{"1x", "1.5x", "2x", "2.5x"};

        if (emu.snapshot().video->newframe) {
            screen.draw(emu.snapshot().video->screen, emu.palette());
        }
        screen.render((scaleSelection / 2.0f) + 1, sdRatio);

        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SetNextItemWidth(aldo::style::glyph_size().x * 10);
            ImGui::Combo("Scale", &scaleSelection, scales.data(),
                         scales.size());
            ImGui::SameLine();
            ImGui::Checkbox("4:3 Ratio", &sdRatio);
        }
    }

private:
    aldo::VideoScreen screen;
    int scaleSelection = 0;
    bool sdRatio = true;
};

}

//
// MARK: - Public Interface
//

void aldo::View::render()
{
    handleTransition();
    if (!visible) return;

    if (ImGui::Begin(title.c_str(), visibility(), options)) {
        renderContents();
    }
    ImGui::End();
}

aldo::Layout::Layout(aldo::viewstate& vs, const aldo::Emulator& emu,
                     const aldo::MediaRuntime& mr)
: vs{vs}, emu{emu}, mr{mr}
{
    add_views<
        CartInfoView,
        CpuView,
        DebuggerView,
        NametablesView,
        PaletteView,
        PatternTablesView,
        PpuView,
        PrgAtPcView,
        RamView,
        SpritesView,
        SystemView,
        VideoView>(views, vs, emu, mr);
}

void aldo::Layout::render() const
{
    aldo::RenderFrame frame{mr, vs.clock.timeRender()};

    main_menu(vs, emu, mr, views);
    for (auto& v : views) {
        v->render();
    }
    about_overlay(vs);
    design_palette(vs);
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
    case aldo::View::Transition::open:
        visible = true;
        break;
    case aldo::View::Transition::close:
        visible = false;
        break;
    case aldo::View::Transition::expand:
        if (visible) {
            ImGui::SetNextWindowCollapsed(false);
        }
        break;
    case aldo::View::Transition::collapse:
        if (visible) {
            ImGui::SetNextWindowCollapsed(true);
        }
        break;
    default:
        break;
    }
}

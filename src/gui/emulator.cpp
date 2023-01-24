//
//  emulator.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/4/22.
//

#include "ctrlsignal.h"
#include "emulator.hpp"
#include "error.hpp"
#include "guiplatform.h"
#include "haltexpr.h"
#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

#include <array>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <cerrno>
#include <cstdio>

#define EXT_BRK "brk"

namespace
{

using file_handle = aldo::handle<std::FILE, std::fclose>;
using hexpr_buffer = std::array<aldo::et::tchar, HEXPR_FMT_SIZE>;
using sdl_buffer = aldo::handle<char, SDL_free>;

auto prefs_path(const gui_platform& p)
{
    const aldo::platform_buffer
        org{p.orgname(), p.free_buffer},
        name{p.appname(), p.free_buffer};
    const sdl_buffer path{SDL_GetPrefPath(org.get(), name.get())};
    if (!path) throw aldo::AldoError{
        "Failed to get preferences path", SDL_GetError(),
    };
    return std::filesystem::path{path.get()};
}

auto invalid_command(aldo::Command c)
{
    std::string s = "Invalid gui event command (";
    s += std::to_string(static_cast<std::underlying_type_t<aldo::Command>>(c));
    s += ')';
    return s;
}

auto is_menu_shortcut(const SDL_Event& ev) noexcept
{
    return (ev.key.keysym.mod & KMOD_GUI) && !ev.key.repeat;
}

auto handle_keydown(const SDL_Event& ev, aldo::viewstate& state,
                    const aldo::EmuController& c)
{
    switch (ev.key.keysym.sym) {
    case SDLK_b:
        if (is_menu_shortcut(ev)) {
            if (ev.key.keysym.mod & KMOD_ALT) {
                if (c.hasDebugState()) {
                    state.events.emplace(aldo::Command::breakpointsExport);
                }
            } else {
                state.events.emplace(aldo::Command::breakpointsOpen);
            }
        }
        break;
    case SDLK_d:
        if (is_menu_shortcut(ev)) {
            state.showDemo = !state.showDemo;
        }
        break;
    case SDLK_o:
        if (is_menu_shortcut(ev)) {
            state.events.emplace(aldo::Command::openROM);
        }
        break;
    }
}

auto brkfile_name(std::filesystem::path cartname)
{
    return cartname.empty()
            ? "breakpoints." EXT_BRK
            : cartname.replace_extension(EXT_BRK);
}

auto prefs_brkpath(const gui_platform& p, std::filesystem::path cartname)
{
    return prefs_path(p) / brkfile_name(std::move(cartname));
}

auto open_file(const gui_platform& p, const char* title,
               std::initializer_list<const char*> filter = {nullptr}) noexcept
{
    return aldo::platform_buffer{
        p.open_file(title, std::data(filter)), p.free_buffer,
    };
}

auto save_file(const gui_platform& p, const char* title,
               const std::filesystem::path& suggestedName) noexcept
{
    return aldo::platform_buffer{
        p.save_file(title, suggestedName.c_str()), p.free_buffer,
    };
}

auto load_debug_state(debugctx* dbg, std::span<debugexpr> exprs) noexcept
{
    debug_reset(dbg);
    for (const auto& expr : exprs) {
        if (expr.type == debugexpr::DBG_EXPR_HALT) {
            debug_bp_add(dbg, expr.hexpr);
        } else {
            debug_set_vector_override(dbg, expr.resetvector);
        }
    }
}

auto format_debug_expr(const debugexpr& expr, hexpr_buffer& buf)
{
    const auto err = haltexpr_fmt_dbgexpr(&expr, buf.data());
    if (err < 0) throw aldo::AldoError{
        "Breakpoint format failure", err, haltexpr_errstr,
    };
}

auto write_debug_line(const hexpr_buffer& buf, std::ofstream& f)
{
    const std::string_view str{buf.data()};
    f.write(str.data(), static_cast<std::streamsize>(str.length()));
    f.put('\n');
}

auto update_bouncer(aldo::viewstate& s, const console_state& snapshot) noexcept
{
    if (!snapshot.lines.ready) return;

    if (s.bouncer.pos.x - s.bouncer.halfdim < 0
        || s.bouncer.pos.x + s.bouncer.halfdim > s.bouncer.bounds.x) {
        s.bouncer.velocity.x *= -1;
    }
    if (s.bouncer.pos.y - s.bouncer.halfdim < 0
        || s.bouncer.pos.y + s.bouncer.halfdim > s.bouncer.bounds.y) {
        s.bouncer.velocity.y *= -1;
    }
    s.bouncer.pos.x += s.bouncer.velocity.x;
    s.bouncer.pos.y += s.bouncer.velocity.y;
}

}

//
// Public Interface
//

std::string_view aldo::EmuController::cartName() const noexcept
{
    // NOTE: not a ternary because the expression would resolve to
    // const std::string& which would create a dangling temporary out of
    // cart_errstr's const char*.
    if (cartname.empty()) return cart_errstr(CART_ERR_NOCART);

    return cartname.native();
}

std::optional<cartinfo> aldo::EmuController::cartInfo() const
{
    if (!hcart) return {};

    cartinfo info;
    cart_getinfo(cartp(), &info);
    return info;
}

void aldo::EmuController::handleInput(aldo::viewstate& state,
                                      const gui_platform& platform)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        switch (ev.type) {
        case SDL_KEYDOWN:
            handle_keydown(ev, state, *this);
            break;
        case SDL_QUIT:
            state.events.emplace(aldo::Command::quit);
            break;
        }
    }
    while (!state.events.empty()) {
        const auto& ev = state.events.front();
        processEvent(ev, state, platform);
        state.events.pop();
    }
}

void aldo::EmuController::update(aldo::viewstate& state) noexcept
{
    nes_cycle(consolep(), &state.clock.cyclock);
    nes_snapshot(consolep(), snapshotp());
    update_bouncer(state, snapshot());
}

void aldo::EmuController::shutdown(const gui_platform& platform)
{
    saveCartState(platform);
}

//
// Private Interface
//

struct aldo::EmuController::file_modal {
    std::function<aldo::platform_buffer(const gui_platform&)> open;
    void (aldo::EmuController::* handle)(const char*, const gui_platform&);
};

void aldo::EmuController::saveCartState(const gui_platform& p)
{
    if (!hcart) return;

    const auto brkPath = prefs_brkpath(p, cartname);
    if (hasDebugState()) {
        exportBreakpointsTo(brkPath.c_str(), p);
    } else {
        try {
            std::filesystem::remove(brkPath);
        } catch (const std::filesystem::filesystem_error& ex) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to delete breakpoint file: (%d) %s",
                        ex.code().value(), ex.what());
        }
    }
}

void aldo::EmuController::loadCartState(const gui_platform &p)
{
    const auto brkPath = prefs_brkpath(p, cartname);
    if (!std::filesystem::exists(brkPath)) return;
    loadBreakpointsFrom(brkPath.c_str(), p);
}

void aldo::EmuController::loadCartFrom(const char* filepath,
                                       const gui_platform& p)
{
    cart* c;
    errno = 0;
    const file_handle f{std::fopen(filepath, "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    const int err = cart_create(&c, f.get());
    if (err < 0) throw aldo::AldoError{"Cart load failure", err, cart_errstr};

    saveCartState(p);
    nes_powerdown(consolep());
    hcart.reset(c);
    nes_powerup(consolep(), cartp(), false);
    cartFilepath = filepath;
    cartname = cartFilepath.stem();
    loadCartState(p);
}

void aldo::EmuController::loadBreakpointsFrom(const char* filepath,
                                              const gui_platform&)
{
    std::ifstream f{filepath};
    if (!f) throw aldo::AldoError{"Cannot open breakpoints file", filepath};

    hexpr_buffer buf;
    std::vector<debugexpr> exprs;
    while (f.getline(buf.data(), buf.size())) {
        debugexpr expr;
        const auto err = haltexpr_parse_dbgexpr(buf.data(), &expr);
        if (err < 0) throw aldo::AldoError{
            "Breakpoints parse failure", err, haltexpr_errstr,
        };
        exprs.push_back(expr);
    }
    // NOTE: defer modifying debug state in case expr parsing throws
    load_debug_state(debugp(), exprs);
}

void aldo::EmuController::exportBreakpointsTo(const char* filepath,
                                              const gui_platform&)
{
    const auto resetvector = resetVectorOverride();
    const auto resOverride = resetvector != NoResetVector;
    const auto exprCount = breakpointCount()
                            + static_cast<aldo::et::size>(resOverride);
    if (exprCount == 0) return;

    std::vector<hexpr_buffer> bufs(exprCount);
    debugexpr expr;
    aldo::et::diff i = 0;
    for (auto bp = breakpointAt(i); bp; bp = breakpointAt(++i)) {
        expr = {.hexpr = bp->expr, .type = debugexpr::DBG_EXPR_HALT};
        format_debug_expr(expr,
                          bufs[static_cast<decltype(bufs)::size_type>(i)]);
    }
    if (resOverride) {
        expr = {.resetvector = resetvector, .type = debugexpr::DBG_EXPR_RESET};
        format_debug_expr(expr, bufs.back());
    }

    std::ofstream f{filepath};
    if (!f) throw aldo::AldoError{"Cannot create breakpoints file", filepath};
    for (const auto& buf : bufs) {
        write_debug_line(buf, f);
    }
}

void aldo::EmuController::openModal(const gui_platform& p,
                                    const aldo::EmuController::file_modal& fm)
{
    // NOTE: halt console to prevent time-jump from modal delay
    // TODO: does this make sense long-term?
    nes_halt(consolep());

    const aldo::platform_buffer filepath = fm.open(p);
    if (!filepath) return;

    SDL_Log("File selected: %s", filepath.get());
    try {
        (this->*fm.handle)(filepath.get(), p);
    } catch (const aldo::AldoError& err) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
        p.display_error(err.title(), err.message());
    }
}

void aldo::EmuController::processEvent(const aldo::event& ev,
                                       aldo::viewstate& s,
                                       const gui_platform& p)
{
    switch (ev.cmd) {
    case aldo::Command::breakpointAdd:
        debug_bp_add(debugp(), std::get<haltexpr>(ev.value));
        break;
    case aldo::Command::breakpointRemove:
        debug_bp_remove(debugp(), std::get<aldo::et::diff>(ev.value));
        break;
    case aldo::Command::breakpointsClear:
        debug_bp_clear(debugp());
        break;
    case aldo::Command::breakpointsExport:
        {
            const auto open =
                [this](const gui_platform& p) -> aldo::platform_buffer {
                    return save_file(p, "Export Breakpoints",
                                     brkfile_name(this->cartname));
                };
            openModal(p, {open, &aldo::EmuController::exportBreakpointsTo});
        }
        break;
    case aldo::Command::breakpointsOpen:
        {
            const auto open =
                [](const gui_platform& p) -> aldo::platform_buffer {
                    return open_file(p, "Choose a Breakpoints file",
                                     {EXT_BRK, nullptr});
                };
            openModal(p, {open, &aldo::EmuController::loadBreakpointsFrom});
        }
        break;
    case aldo::Command::breakpointToggle:
        {
            const auto idx = std::get<aldo::et::diff>(ev.value);
            const auto bp = breakpointAt(idx);
            debug_bp_enabled(debugp(), idx, bp && !bp->enabled);
        }
        break;
    case aldo::Command::halt:
        if (std::get<bool>(ev.value)) {
            nes_halt(consolep());
        } else {
            nes_ready(consolep());
        }
        break;
    case aldo::Command::interrupt:
        {
            const auto [signal, active] =
                std::get<aldo::event::interrupt>(ev.value);
            if (active) {
                nes_interrupt(consolep(), signal);
            } else {
                nes_clear(consolep(), signal);
            }
        }
        break;
    case aldo::Command::launchStudio:
        p.launch_studio();
        break;
    case aldo::Command::mode:
        nes_set_mode(consolep(), std::get<csig_excmode>(ev.value));
        break;
    case aldo::Command::openROM:
        {
            const auto open =
                [](const gui_platform& p) -> aldo::platform_buffer {
                    return open_file(p, "Choose a ROM file");
                };
            openModal(p, {open, &aldo::EmuController::loadCartFrom});
        }
        break;
    case aldo::Command::overrideReset:
        debug_set_vector_override(debugp(), std::get<int>(ev.value));
        break;
    case aldo::Command::quit:
        s.running = false;
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}

//
//  emu.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#include "emu.hpp"

#include "attr.hpp"
#include "error.hpp"
#include "guiplatform.h"

#include <SDL2/SDL.h>

#include <array>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>
#include <cerrno>
#include <cstdio>

namespace
{

constexpr const char* FileErrorTitle = "File Error";

using file_handle = aldo::handle<std::FILE, std::fclose>;
using hexpr_buffer = std::array<aldo::et::tchar, HEXPR_FMT_SIZE>;
using sdl_buffer = aldo::handle<char, SDL_free>;

auto get_prefspath(const gui_platform& p)
{
    const aldo::platform_buffer
        org{p.orgname(), p.free_buffer},
        name{p.appname(), p.free_buffer};
    const sdl_buffer path{SDL_GetPrefPath(org.get(), name.get())};
    if (!path) throw aldo::SdlError{"Failed to get preferences path"};
    return std::filesystem::path{path.get()};
}

auto read_brkfile(const std::filesystem::path& filepath)
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
    if (f.bad() || (f.fail() && !f.eof())) throw aldo::AldoError{
        FileErrorTitle, "Error reading breakpoints file"
    };
    return exprs;
}

auto set_debug_state(debugctx* dbg, std::span<const debugexpr> exprs) noexcept
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

auto write_brkline(const hexpr_buffer& buf, std::ofstream& f)
{
    const std::string_view str{buf.data()};
    f.write(str.data(), static_cast<std::streamsize>(str.length()));
    f.put('\n');
    if (f.bad()) throw aldo::AldoError{
        FileErrorTitle, "Error writing breakpoints file",
    };
}

auto write_brkfile(const std::filesystem::path& filepath,
                   std::span<const hexpr_buffer> bufs)
{
    std::ofstream f{filepath};
    if (!f) throw aldo::AldoError{"Cannot create breakpoints file", filepath};
    for (const auto& buf : bufs) {
        write_brkline(buf, f);
    }
}

ALDO_OWN
auto load_cart(const std::filesystem::path& filepath)
{
    cart* c;
    errno = 0;
    const file_handle f{std::fopen(filepath.c_str(), "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    const int err = cart_create(&c, f.get());
    if (err < 0) throw aldo::AldoError{"Cart load failure", err, cart_errstr};

    return c;
}

}

void
aldo::Debugger::loadBreakpoints(const std::filesystem::path& filepath) const
{
    const auto exprs = read_brkfile(filepath);
    set_debug_state(debugp(), exprs);
}

void
aldo::Debugger::exportBreakpoints(const std::filesystem::path& filepath) const
{
    const auto resetvector = vectorOverride();
    const auto resOverride = resetvector != NoResetVector;
    const auto exprCount = breakpointCount()
                            + static_cast<aldo::et::size>(resOverride);
    if (exprCount == 0) return;

    std::vector<hexpr_buffer> bufs(exprCount);
    debugexpr expr;
    aldo::et::diff i = 0;
    // TODO: factor into helper function when getBreakpoint moved to public
    auto it = bufs.begin();
    for (auto bp = getBreakpoint(i); bp; bp = getBreakpoint(++i), ++it) {
        expr = {.hexpr = bp->expr, .type = debugexpr::DBG_EXPR_HALT};
        format_debug_expr(expr, *it);
    }
    if (resOverride) {
        expr = {.resetvector = resetvector, .type = debugexpr::DBG_EXPR_RESET};
        format_debug_expr(expr, bufs.back());
    }

    write_brkfile(filepath, bufs);
}

aldo::Emulator::Emulator(debug_handle d, console_handle n,
                         const gui_platform& p)
: prefspath{get_prefspath(p)}, hdebug{std::move(d)}, hconsole{std::move(n)},
    hsnapshot{consolep()} {}

void aldo::Emulator::loadCart(const std::filesystem::path& filepath)
{
    const auto c = load_cart(filepath);
    //saveCartState(p);
    nes_powerdown(consolep());
    hcart.reset(c);
    nes_powerup(consolep(), cartp(), false);
    cartpath = filepath;
    cartname = cartpath.stem();
    //loadCartState(p);
}

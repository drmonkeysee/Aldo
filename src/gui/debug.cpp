//
//  debug.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/27/23.
//

#include "debug.hpp"

#include "error.hpp"

#include <SDL2/SDL.h>

#include <array>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

namespace
{

using hexpr_buffer = std::array<aldo::et::tchar, HEXPR_FMT_SIZE>;
using bp_it = aldo::Debugger::BreakpointIterator;
using bp_size = aldo::Debugger::BpView::size_type;

constexpr auto DebugFileErrorTitle = "Debug File Error";

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
        DebugFileErrorTitle, "Error reading breakpoints file"
    };
    return exprs;
}

auto set_debug_state(debugctx* dbg, std::span<const debugexpr> exprs) noexcept
{
    debug_reset(dbg);
    for (auto& expr : exprs) {
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

auto fill_expr_buffers(bp_size exprCount, int resetvector, bool resOverride,
                       bp_it first, bp_it last)
{
    std::vector<hexpr_buffer> bufs(exprCount);
    debugexpr expr;
    for (auto it = bufs.begin(); first != last; ++first, ++it) {
        expr = {.hexpr = first->expr, .type = debugexpr::DBG_EXPR_HALT};
        format_debug_expr(expr, *it);
    }
    if (resOverride) {
        expr = {.resetvector = resetvector, .type = debugexpr::DBG_EXPR_RESET};
        format_debug_expr(expr, bufs.back());
    }
    return bufs;
}

auto write_brkline(const hexpr_buffer& buf, std::ofstream& f)
{
    const std::string_view str{buf.data()};
    f.write(str.data(), static_cast<std::streamsize>(str.length()));
    f.put('\n');
    if (f.bad()) throw aldo::AldoError{
        DebugFileErrorTitle, "Error writing breakpoints file",
    };
}

auto write_brkfile(const std::filesystem::path& filepath,
                   std::span<const hexpr_buffer> bufs)
{
    std::ofstream f{filepath};
    if (!f) throw aldo::AldoError{"Cannot create breakpoints file", filepath};
    for (auto& buf : bufs) {
        write_brkline(buf, f);
    }
}

}

void aldo::Debugger::loadBreakpoints(const std::filesystem::path& filepath)
{
    set_debug_state(debugp(), read_brkfile(filepath));
}

void
aldo::Debugger::exportBreakpoints(const std::filesystem::path& filepath) const
{
    const auto bpView = breakpoints();
    const auto resOverride = isVectorOverridden();
    const auto exprCount = bpView.size() + static_cast<bp_size>(resOverride);
    if (exprCount == 0) return;

    const auto bufs = fill_expr_buffers(exprCount, vectorOverride(),
                                        resOverride, bpView.cbegin(),
                                        bpView.cend());
    write_brkfile(filepath, bufs);
}

void aldo::Debugger::loadCartState(const std::filesystem::path& prefCartPath)
{
    const auto brkpath = aldo::debug::breakfile_path_from(prefCartPath);
    if (!std::filesystem::exists(brkpath)) return;
    loadBreakpoints(brkpath);
}

void
aldo::Debugger::saveCartState(const std::filesystem::path& prefCartPath) const
{
    const auto brkpath = aldo::debug::breakfile_path_from(prefCartPath);
    if (isActive()) {
        exportBreakpoints(brkpath);
    } else {
        try {
            std::filesystem::remove(brkpath);
        } catch (const std::filesystem::filesystem_error& ex) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to delete breakpoint file: (%d) %s",
                        ex.code().value(), ex.what());
        }
    }
}

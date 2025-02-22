//
//  debug.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/27/23.
//

#include "debug.hpp"

#include "error.hpp"

#include <SDL2/SDL.h>

#include <fstream>
#include <span>
#include <string_view>
#include <vector>
#include <cerrno>

namespace
{

using bp_it = aldo::Debugger::BreakpointIterator;
using bp_sz = aldo::Debugger::BpView::size_type;

constexpr auto DebugFileErrorTitle = "Debug File Error";

auto read_brkfile(const std::filesystem::path& filepath)
{
    std::ifstream f{filepath};
    if (!f) throw aldo::AldoError{"Cannot open breakpoints file", filepath};

    aldo::hexpr_buffer buf;
    std::vector<aldo_debugexpr> exprs;
    while (f.getline(buf.data(), buf.size())) {
        aldo_debugexpr expr;
        auto err = aldo_haltexpr_parse_dbg(buf.data(), &expr);
        if (err < 0) throw aldo::AldoError{
            "Breakpoints parse failure", err, aldo_haltexpr_errstr,
        };
        exprs.push_back(expr);
    }
    if (f.bad() || (f.fail() && !f.eof())) throw aldo::AldoError{
        DebugFileErrorTitle, "Error reading breakpoints file"
    };
    return exprs;
}

auto set_debug_state(aldo_debugger* dbg, std::span<const aldo_debugexpr> exprs)
{
    aldo_debug_reset(dbg);
    for (auto& expr : exprs) {
        if (expr.type == aldo_debugexpr::ALDO_DBG_EXPR_HALT) {
            if (!aldo_debug_bp_add(dbg, expr.hexpr)) throw aldo::AldoError{
                "Cannot add breakpoint", "System error", errno,
            };
        } else {
            aldo_debug_set_vector_override(dbg, expr.resetvector);
        }
    }
}

auto format_debug_expr(const aldo_debugexpr& expr, aldo::hexpr_buffer& buf)
{
    auto err = aldo_haltexpr_fmtdbg(&expr, buf.data());
    if (err < 0) throw aldo::AldoError{
        "Breakpoint format failure", err, aldo_haltexpr_errstr,
    };
}

auto fill_expr_buffers(bp_sz exprCount, int resetvector, bool resOverride,
                       bp_it first, bp_it last)
{
    std::vector<aldo::hexpr_buffer> bufs(exprCount);
    aldo_debugexpr expr;
    for (auto it = bufs.begin(); first != last; ++first, ++it) {
        expr = {
            .hexpr = first->expr,
            .type = aldo_debugexpr::ALDO_DBG_EXPR_HALT,
        };
        format_debug_expr(expr, *it);
    }
    if (resOverride) {
        expr = {
            .resetvector = resetvector,
            .type = aldo_debugexpr::ALDO_DBG_EXPR_RESET,
        };
        format_debug_expr(expr, bufs.back());
    }
    return bufs;
}

auto write_brkline(const aldo::hexpr_buffer& buf, std::ofstream& f)
{
    std::string_view str{buf.data()};
    f.write(str.data(), static_cast<std::streamsize>(str.length()));
    f.put('\n');
    if (f.bad()) throw aldo::AldoError{
        DebugFileErrorTitle, "Error writing breakpoints file",
    };
}

auto write_brkfile(const std::filesystem::path& filepath,
                   std::span<const aldo::hexpr_buffer> bufs)
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
    set_debug_state(dbgp(), read_brkfile(filepath));
}

void
aldo::Debugger::exportBreakpoints(const std::filesystem::path& filepath) const
{
    auto bpView = breakpoints();
    auto resOverride = isVectorOverridden();
    auto exprCount = bpView.size() + static_cast<bp_sz>(resOverride);
    if (exprCount == 0) return;

    auto bufs = fill_expr_buffers(exprCount, vectorOverride(), resOverride,
                                  bpView.cbegin(), bpView.cend());
    write_brkfile(filepath, bufs);
}

void aldo::Debugger::loadCartState(const std::filesystem::path& prefCartPath)
{
    auto brkpath = aldo::debug::breakfile_path_from(prefCartPath);
    if (!std::filesystem::exists(brkpath)) return;
    loadBreakpoints(brkpath);
    SDL_Log("Breakpoints file loaded: %s", brkpath.c_str());
}

void
aldo::Debugger::saveCartState(const std::filesystem::path& prefCartPath) const
{
    auto brkpath = aldo::debug::breakfile_path_from(prefCartPath);
    if (isActive()) {
        exportBreakpoints(brkpath);
        SDL_Log("Breakpoints file saved: %s", brkpath.c_str());
    } else {
        try {
            std::filesystem::remove(brkpath);
        } catch (const std::filesystem::filesystem_error& ex) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to delete breakpoint file: (%d) %s",
                        ex.code().value(), ex.what());
        }
        SDL_Log("Empty breakpoints file removed: %s", brkpath.c_str());
    }
}

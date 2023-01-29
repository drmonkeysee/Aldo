//
//  debug.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/27/23.
//

#include "debug.hpp"

#include "error.hpp"

#include <array>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

namespace
{

constexpr const char* DebugFileErrorTitle = "Debug File Error";

using hexpr_buffer = std::array<aldo::et::tchar, HEXPR_FMT_SIZE>;
using bp_size = aldo::Debugger::bp_collection::size_type;
using bp_it = aldo::Debugger::bp_collection::const_iterator;

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

auto fill_expr_buffers(bp_size exprCount, int resetvector, bool resOverride,
                       bp_it start, bp_it end)
{
    std::vector<hexpr_buffer> bufs(exprCount);
    debugexpr expr;
    for (auto it = bufs.begin(); start != end; ++start, ++it) {
        expr = {.hexpr = start->expr, .type = debugexpr::DBG_EXPR_HALT};
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
    for (const auto& buf : bufs) {
        write_brkline(buf, f);
    }
}

}

void aldo::Debugger::loadBreakpoints(const std::filesystem::path& filepath)
{
    const auto exprs = read_brkfile(filepath);
    set_debug_state(debugp(), exprs);
}

void
aldo::Debugger::exportBreakpoints(const std::filesystem::path& filepath) const
{
    const auto& bpColl = breakpoints();
    const auto resetvector = vectorOverride();
    const auto resOverride = resetvector != NoResetVector;
    const auto exprCount = bpColl.size() + static_cast<bp_size>(resOverride);
    if (exprCount == 0) return;

    const auto bufs = fill_expr_buffers(exprCount, resetvector, resOverride,
                                        bpColl.cbegin(), bpColl.cend());
    write_brkfile(filepath, bufs);
}

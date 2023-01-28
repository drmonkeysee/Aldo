//
//  debug.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/27/23.
//

#ifndef Aldo_gui_debug_hpp
#define Aldo_gui_debug_hpp

#include "debug.h"
#include "emutypes.hpp"
#include "haltexpr.h"
#include "handle.hpp"

#include <filesystem>
#include <utility>

namespace aldo
{

using debug_handle = handle<debugctx, debug_free>;

class Debugger {
public:
    explicit Debugger(debug_handle d) noexcept : hdebug{std::move(d)} {}

    void vectorOverride(int resetvector) const noexcept
    {
        debug_set_vector_override(debugp(), resetvector);
    }
    void addBreakpoint(haltexpr expr) const noexcept
    {
        debug_bp_add(debugp(), expr);
    }
    void toggleBreakpointEnabled(et::diff at) const noexcept
    {
        const auto bp = getBreakpoint(at);
        debug_bp_enabled(debugp(), at, bp && !bp->enabled);
    }
    void removeBreakpoint(et::diff at) const noexcept
    {
        debug_bp_remove(debugp(), at);
    }
    void clearBreakpoints() const noexcept { debug_bp_clear(debugp()); }
    bool isActive() const noexcept
    {
        return vectorOverride() != NoResetVector || breakpointCount() > 0;
    }

    void loadBreakpoints(const std::filesystem::path& filepath) const;
    void exportBreakpoints(const std::filesystem::path& filepath) const;

private:
    int vectorOverride() const noexcept
    {
        return debug_vector_override(debugp());
    }
    et::size breakpointCount() const noexcept
    {
        return debug_bp_count(debugp());
    }
    const breakpoint* getBreakpoint(et::diff at) const noexcept
    {
        return debug_bp_at(debugp(), at);
    }

    debugctx* debugp() const noexcept { return hdebug.get(); }

    debug_handle hdebug;
};

}

#endif

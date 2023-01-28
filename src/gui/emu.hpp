//
//  emu.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#ifndef Aldo_gui_emu_hpp
#define Aldo_gui_emu_hpp

#include "cart.h"
#include "ctrlsignal.h"
#include "debug.h"
#include "emutypes.hpp"
#include "haltexpr.h"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <filesystem>
#include <utility>

struct gui_platform;

namespace aldo
{

using cart_handle = handle<cart, cart_free>;
using console_handle = handle<nes, nes_free>;
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

class Snapshot {
public:
    explicit Snapshot(nes* console) noexcept { nes_snapshot(console, getp()); }
    Snapshot(const Snapshot&) = default;
    Snapshot& operator=(const Snapshot&) = default;
    Snapshot(Snapshot&&) = default;
    Snapshot& operator=(Snapshot&&) = default;
    ~Snapshot() { snapshot_clear(getp()); }

private:
    console_state* getp() noexcept { return &snapshot; }

    console_state snapshot;
};

class Emulator {
public:
    Emulator(debug_handle d, console_handle n, const gui_platform& p);

    void halt() const noexcept { nes_halt(consolep()); }
    void ready() const noexcept { nes_ready(consolep()); }
    void interrupt(csig_interrupt signal, bool active) const noexcept
    {
        if (active) {
            nes_interrupt(consolep(), signal);
        } else {
            nes_clear(consolep(), signal);
        }
    }
    void runMode(csig_excmode mode) const noexcept
    {
        nes_set_mode(consolep(), mode);
    }

    void loadCart(const std::filesystem::path& filepath);

    const std::filesystem::path& cartName() const noexcept { return cartname; }
    const Debugger& debugger() const noexcept { return hdebug; }

private:
    cart* cartp() const noexcept { return hcart.get(); }
    nes* consolep() const noexcept { return hconsole.get(); }

    std::filesystem::path cartpath;
    std::filesystem::path cartname;
    std::filesystem::path prefspath;
    cart_handle hcart;
    Debugger hdebug;
    console_handle hconsole;
    Snapshot hsnapshot;
};

}

#endif
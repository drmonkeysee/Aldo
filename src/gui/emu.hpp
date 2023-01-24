//
//  emu.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#ifndef Aldo_gui_emu_hpp
#define Aldo_gui_emu_hpp

#include "cart.h"
#include "debug.h"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <filesystem>
#include <utility>

namespace aldo
{

using cart_handle = handle<cart, cart_free>;
using console_handle = handle<nes, nes_free>;
using debug_handle = handle<debugctx, debug_free>;

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
    Emulator(debug_handle d, console_handle n) noexcept
    : hdebug{std::move(d)}, hconsole{std::move(n)}, hsnapshot{consolep()} {}

private:
    nes* consolep() const noexcept { return hconsole.get(); }

    std::filesystem::path cartpath;
    std::filesystem::path cartname;
    cart_handle hcart;
    debug_handle hdebug;
    console_handle hconsole;
    Snapshot hsnapshot;
};

}

#endif

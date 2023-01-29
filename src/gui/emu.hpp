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
#include "debug.hpp"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <filesystem>

struct gui_platform;

namespace aldo
{

using cart_handle = handle<cart, cart_free>;
using console_handle = handle<nes, nes_free>;

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
    Emulator(Debugger d, console_handle n, const gui_platform& p);

    void halt() noexcept { nes_halt(consolep()); }
    void ready() noexcept { nes_ready(consolep()); }
    void interrupt(csig_interrupt signal, bool active) noexcept
    {
        if (active) {
            nes_interrupt(consolep(), signal);
        } else {
            nes_clear(consolep(), signal);
        }
    }
    void runMode(csig_excmode mode) noexcept
    {
        nes_set_mode(consolep(), mode);
    }

    void loadCart(const std::filesystem::path& filepath);

    const std::filesystem::path& cartName() const noexcept { return cartname; }
    const Debugger& debugger() const noexcept { return hdebug; }
    Debugger& debugger() noexcept { return hdebug; }

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

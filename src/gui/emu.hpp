//
//  emu.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#ifndef Aldo_gui_emu_hpp
#define Aldo_gui_emu_hpp

#include "attr.hpp"
#include "cart.h"
#include "cpu.h"
#include "ctrlsignal.h"
#include "debug.hpp"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <filesystem>
#include <optional>
#include <string_view>

struct gui_platform;

namespace aldo
{

struct viewstate;

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

    const console_state& get() const noexcept { return snapshot; }
    const console_state* getp() const noexcept { return &snapshot; }
    console_state* getp() noexcept { return &snapshot; }

private:
    console_state snapshot;
};

class ALDO_SIDEFX Emulator {
public:
    inline static const int MaxCpuTCycle = MaxTCycle;

    Emulator(Debugger d, console_handle n, const gui_platform& p);
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;
    ~Emulator() { cleanup(); }

    const std::filesystem::path& cartName() const noexcept { return cartname; }
    std::string_view displayCartName() const noexcept;
    std::optional<cartinfo> cartInfo() const;
    const Debugger& debugger() const noexcept { return hdebug; }
    Debugger& debugger() noexcept { return hdebug; }
    const console_state& snapshot() const noexcept { return hsnapshot.get(); }
    const console_state* snapshotp() const noexcept
    {
        return hsnapshot.getp();
    }
    bool haltedByDebugger() const noexcept
    {
        return debugger().hasBreak(snapshot());
    }

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
    enum csig_excmode runMode() const noexcept { return nes_mode(consolep()); }
    void runMode(csig_excmode mode) noexcept
    {
        nes_set_mode(consolep(), mode);
    }

    void loadCart(const std::filesystem::path& filepath);
    void update(viewstate& vs) noexcept;

private:
    cart* cartp() const noexcept { return hcart.get(); }
    nes* consolep() const noexcept { return hconsole.get(); }
    console_state* snapshotp() noexcept { return hsnapshot.getp(); }

    void loadCartState();
    void saveCartState() const;
    void cleanup() const noexcept;

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

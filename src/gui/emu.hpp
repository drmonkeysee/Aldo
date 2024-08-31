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
#include "ctrlsignal.h"
#include "debug.hpp"
#include "handle.hpp"
#include "nes.h"
#include "palette.hpp"
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
    explicit Snapshot(nes* console) noexcept
    {
        snapshot_extend(getp());
        nes_snapshot(console, getp());
    }
    Snapshot(const Snapshot&) = default;
    Snapshot& operator=(const Snapshot&) = default;
    Snapshot(Snapshot&&) = default;
    Snapshot& operator=(Snapshot&&) = default;
    ~Snapshot() { snapshot_cleanup(getp()); }

    const snapshot& get() const noexcept { return snp; }
    const snapshot* getp() const noexcept { return &snp; }

private:
    friend class Emulator;

    snapshot* getp() noexcept { return &snp; }

    snapshot snp{};
};

class ALDO_SIDEFX Emulator {
public:
    using console_snapshot = snapshot;

    Emulator(debug_handle d, console_handle n, const gui_platform& p);
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;
    ~Emulator() { cleanup(); }

    const std::filesystem::path& cartName() const noexcept { return cartname; }
    std::string_view displayCartName() const noexcept;
    std::optional<cartinfo> cartInfo() const;
    const Debugger& debugger() const noexcept { return hdbg; }
    Debugger& debugger() noexcept { return hdbg; }
    const console_snapshot& snapshot() const noexcept { return hsnp.get(); }
    const console_snapshot* snapshotp() const noexcept { return hsnp.getp(); }
    const Palette& palette() const noexcept { return hpalette; }
    Palette& palette() noexcept { return hpalette; }
    bool haltedByDebugger() const noexcept
    {
        return debugger().hasBreak(snapshot());
    }

    void ready(bool ready) noexcept { nes_ready(consolep(), ready); }
    et::size ramSize() const noexcept { return nes_ram_size(consolep()); }
    bool bcdSupport() const noexcept { return nes_bcd_support(consolep()); }
    csig_excmode runMode() const noexcept { return nes_mode(consolep()); }
    void runMode(csig_excmode mode) noexcept
    {
        nes_set_mode(consolep(), mode);
    }
    bool probe(csig_interrupt signal) const noexcept
    {
        return nes_probe(consolep(), signal);
    }
    void probe(csig_interrupt signal, bool active) noexcept
    {
        nes_set_probe(consolep(), signal, active);
    }

    void loadCart(const std::filesystem::path& filepath);
    void update(viewstate& vs) noexcept;

    bool zeroRam = false;

private:
    cart* cartp() const noexcept { return hcart.get(); }
    nes* consolep() const noexcept { return hconsole.get(); }
    console_snapshot* snapshotp() noexcept { return hsnp.getp(); }

    void loadCartState();
    void saveCartState() const;
    void cleanup() const noexcept;

    std::filesystem::path cartname, cartpath, prefspath;
    cart_handle hcart;
    Debugger hdbg;
    console_handle hconsole;
    Snapshot hsnp;
    Palette hpalette;
};

}

#endif

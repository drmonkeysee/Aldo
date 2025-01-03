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
#include "emutypes.hpp"
#include "error.hpp"
#include "handle.hpp"
#include "nes.h"
#include "palette.hpp"
#include "snapshot.h"

#include <SDL2/SDL.h>

#include <filesystem>
#include <optional>
#include <string_view>
#include <cerrno>

struct gui_platform;

namespace aldo
{

struct viewstate;

using cart_handle = handle<aldo_cart, aldo_cart_free>;
using console_handle = handle<aldo_nes, aldo_nes_free>;

class Snapshot {
public:
    Snapshot()
    {
        if (!aldo_snapshot_extend(getp())) throw AldoError{
            "Unable to extend snapshot", "System error", errno,
        };
    }
    Snapshot(const Snapshot&) = delete;
    Snapshot& operator=(const Snapshot&) = delete;
    Snapshot(Snapshot&&) = delete;
    Snapshot& operator=(Snapshot&&) = delete;
    ~Snapshot() { aldo_snapshot_cleanup(getp()); }

    const aldo_snapshot& get() const noexcept { return snp; }
    const aldo_snapshot* getp() const noexcept { return &snp; }

private:
    friend class Emulator;

    aldo_snapshot* getp() noexcept { return &snp; }

    aldo_snapshot snp{};
};

class ALDO_SIDEFX Emulator {
public:
    static SDL_Point screenSize() noexcept
    {
        SDL_Point res;
        aldo_nes_screen_size(&res.x, &res.y);
        return res;
    }

    Emulator(debug_handle d, console_handle c, const gui_platform& p);
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;
    ~Emulator() { cleanup(); }

    const std::filesystem::path& cartName() const noexcept { return cartname; }
    std::string_view displayCartName() const noexcept;
    std::optional<aldo_cartinfo> cartInfo() const;
    const Debugger& debugger() const noexcept { return hdbg; }
    Debugger& debugger() noexcept { return hdbg; }
    const aldo_snapshot& snapshot() const noexcept { return hsnp.get(); }
    const aldo_snapshot* snapshotp() const noexcept { return hsnp.getp(); }
    const Palette& palette() const noexcept { return hpalette; }
    Palette& palette() noexcept { return hpalette; }

    void ready(bool ready) noexcept { aldo_nes_ready(consolep(), ready); }
    et::size ramSize() const noexcept { return aldo_nes_ram_size(consolep()); }
    bool bcdSupport() const noexcept
    {
        return aldo_nes_bcd_support(consolep());
    }
    aldo_execmode runMode() const noexcept
    {
        return aldo_nes_mode(consolep());
    }
    void runMode(aldo_execmode mode) noexcept
    {
        aldo_nes_set_mode(consolep(), mode);
    }
    bool probe(aldo_interrupt signal) const noexcept
    {
        return aldo_nes_probe(consolep(), signal);
    }
    void probe(aldo_interrupt signal, bool active) noexcept
    {
        aldo_nes_set_probe(consolep(), signal, active);
    }

    void loadCart(const std::filesystem::path& filepath);
    void update(viewstate& vs) noexcept;

    bool zeroRam = false;

private:
    aldo_cart* cartp() const noexcept { return hcart.get(); }
    aldo_nes* consolep() const noexcept { return hconsole.get(); }
    aldo_snapshot* snapshotp() noexcept { return hsnp.getp(); }

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

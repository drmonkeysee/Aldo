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
#include "console.h"
#include "ctrlsignal.h"
#include "debug.hpp"
#include "emutypes.hpp"
#include "error.hpp"
#include "handle.hpp"
#include "palette.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <optional>
#include <string_view>
#include <cerrno>

struct gui_platform;

namespace aldo
{

class Emulator;
struct viewstate;
using cart_handle = handle<aldo_cart, aldo_cart_free>;
using console_handle = handle<aldo_console, aldo_console_free>;

class ALDO_SIDEFX Emulator {
public:
    static int maxTCpu() noexcept { return aldo_console_max_tcpu(); }

    Emulator(debug_handle d, cart_handle c, console_handle cn, const gui_platform& p);
    ~Emulator() { cleanup(); }

    std::string_view name() const noexcept { return aldo_console_name(consolep()); }
    const std::filesystem::path& cartName() const noexcept { return cartname; }
    std::string_view displayCartName() const noexcept
    {
        if (cartName().empty()) return aldo_cart_errstr(ALDO_CART_ERR_NOCART);
        return cartName().native();
    }
    std::optional<aldo_cartinfo> cartInfo() const
    {
        aldo_cartinfo info;
        aldo_cart_getinfo(cartp(), &info);
        return info;
    }
    const Debugger& debugger() const noexcept { return hdbg; }
    Debugger& debugger() noexcept { return hdbg; }
    const aldo_snapshot& snapshot() const noexcept { return *snapshotp(); }
    const aldo_snapshot* snapshotp() const noexcept { return aldo_console_snapshot(hconsole.get()); }
    const Palette& palette() const noexcept { return hpalette; }
    Palette& palette() noexcept { return hpalette; }

    bool halted() const noexcept { return aldo_console_halted(consolep()); }
    void halt(bool halt) noexcept { aldo_console_halt(consolep(), halt); }
    et::size ramSize() const noexcept { return aldo_console_ram_size(consolep()); }
    SDL_Point screenSize() const noexcept
    {
        SDL_Point res;
        aldo_console_screen_size(consolep(), &res.x, &res.y);
        return res;
    }
    int cycleFactor() const noexcept { return aldo_console_cycle_factor(consolep()); }
    int frameFactor() const noexcept { return aldo_console_frame_factor(consolep()); }
    bool bcdSupport() const noexcept { return aldo_console_bcd_support(consolep()); }
    aldo_execmode runMode() const noexcept { return aldo_console_mode(consolep()); }
    void runMode(aldo_execmode mode) noexcept
    {
        aldo_console_set_mode(consolep(), mode);
    }
    bool probe(aldo_interrupt signal) const noexcept
    {
        return aldo_console_probe(consolep(), signal);
    }
    void probe(aldo_interrupt signal, bool active) noexcept
    {
        aldo_console_set_probe(consolep(), signal, active);
    }

    void loadCart(const std::filesystem::path& filepath);
    void update(viewstate& vs) noexcept;

    bool zeroRam = false;

private:
    aldo_cart* cartp() const noexcept { return hcart.get(); }
    aldo_console* consolep() const noexcept { return hconsole.get(); }

    void loadCartState();
    void saveCartState() const;
    void reloadConsole(aldo_cart* c);
    void cleanup() const noexcept;

    std::filesystem::path cartname, cartpath, prefspath;
    Debugger hdbg;
    cart_handle hcart;
    console_handle hconsole;
    Palette hpalette;
};

}

#endif

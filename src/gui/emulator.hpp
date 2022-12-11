//
//  emulator.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/4/22.
//

#ifndef Aldo_gui_emulator_hpp
#define Aldo_gui_emulator_hpp

#include "cart.h"
#include "debug.h"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <string>
#include <string_view>
#include <utility>

struct gui_platform;

namespace aldo
{

struct event;
struct viewstate;

using cart_handle = handle<cart, cart_free>;
using console_handle = handle<nes, nes_free>;
using debug_handle = handle<debugctx, debug_free>;

class SnapshotScope final {
public:
    explicit SnapshotScope(nes* console) noexcept
    {
        nes_snapshot(console, getp());
    }
    ~SnapshotScope() { snapshot_clear(getp()); }

    const console_state& get() const noexcept { return snapshot; }
    console_state& get() noexcept { return snapshot; }
    console_state* getp() noexcept { return &snapshot; }

private:
    console_state snapshot;
};

class EmuController final {
public:
    EmuController(debug_handle d, console_handle n) noexcept
    : hdebug{std::move(d)}, hconsole{std::move(n)}, lsnapshot{hconsole.get()}
    {}

    std::string_view cartName() const;
    cart* cartp() const noexcept { return hcart.get(); }
    const console_state& snapshot() const noexcept { return lsnapshot.get(); }
    console_state& snapshot() noexcept { return lsnapshot.get(); }

    void handleInput(viewstate& state, const gui_platform& platform);
    void update(viewstate& state) noexcept;

private:
    console_state* snapshotp() noexcept { return lsnapshot.getp(); }

    void loadCartFrom(const char*);
    void openCartFile(const gui_platform&);
    void processEvent(const event&, viewstate&, const gui_platform&);
    void updateBouncer(viewstate&) const noexcept;

    std::string cartFilepath;
    debug_handle hdebug;
    cart_handle hcart;
    console_handle hconsole;
    SnapshotScope lsnapshot;
};

}

#endif

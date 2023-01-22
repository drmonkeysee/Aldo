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
#include "emutypes.hpp"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <filesystem>
#include <optional>
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

class [[nodiscard("raii type")]] SnapshotScope {
public:
    explicit SnapshotScope(nes* console) noexcept
    {
        nes_snapshot(console, getp());
    }
    SnapshotScope(const SnapshotScope&) = default;
    SnapshotScope& operator=(const SnapshotScope&) = default;
    SnapshotScope(SnapshotScope&&) = default;
    SnapshotScope& operator=(SnapshotScope&&) = default;
    ~SnapshotScope() { snapshot_clear(getp()); }

    const console_state& get() const noexcept { return snapshot; }
    const console_state* getp() const noexcept { return &snapshot; }
    console_state* getp() noexcept { return &snapshot; }

private:
    console_state snapshot;
};

class EmuController {
public:
    EmuController(debug_handle d, console_handle n) noexcept
    : hdebug{std::move(d)}, hconsole{std::move(n)},
        lsnapshot{consolep()} {}

    std::string_view cartName() const noexcept;
    std::optional<cartinfo> cartInfo() const;
    const console_state& snapshot() const noexcept { return lsnapshot.get(); }
    const console_state* snapshotp() const noexcept
    {
        return lsnapshot.getp();
    }

    int resetVectorOverride() const noexcept
    {
        return debug_resetvector(debugp());
    }
    et::size breakpointCount() const noexcept
    {
        return debug_bp_count(debugp());
    }
    const breakpoint* breakpointAt(et::diff at) const noexcept
    {
        return debug_bp_at(debugp(), at);
    }
    bool hasDebugState() const noexcept
    {
        return resetVectorOverride() != NoResetVector || breakpointCount() > 0;
    }

    csig_excmode runMode() const noexcept { return nes_mode(consolep()); }

    void handleInput(viewstate& state, const gui_platform& platform);
    void update(viewstate& state) noexcept;

private:
    struct file_modal;

    debugctx* debugp() const noexcept { return hdebug.get(); }
    cart* cartp() const noexcept { return hcart.get(); }
    nes* consolep() const noexcept { return hconsole.get(); }
    console_state* snapshotp() noexcept { return lsnapshot.getp(); }

    void loadCartFrom(const char*);
    void loadBreakpointsFrom(const char*);
    void exportBreakpointsTo(const char*);
    void openModal(const gui_platform&, const file_modal&);
    void processEvent(const event&, viewstate&, const gui_platform&);

    std::filesystem::path cartFilepath;
    std::filesystem::path cartname;
    debug_handle hdebug;
    cart_handle hcart;
    console_handle hconsole;
    SnapshotScope lsnapshot;
};

}

#endif

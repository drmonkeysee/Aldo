//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include "ctrlsignal.h"
#include "emutypes.hpp"
#include "haltexpr.h"
#include "palette.hpp"
#include "runclock.hpp"

#include <concepts>
#include <queue>
#include <utility>
#include <variant>

namespace aldo
{

enum class Command {
    breakpointAdd,
    breakpointDisable,
    breakpointEnable,
    breakpointRemove,
    breakpointsClear,
    breakpointsExport,
    breakpointsOpen,
    mode,
    openROM,
    paletteLoad,
    paletteUnload,
    probe,
    ready,
    resetVectorClear,
    resetVectorOverride,
    quit,
    zeroRamOnPowerup,
};

struct command_state {
    using probe = std::pair<aldo_interrupt, bool>;
    using payload =
        std::variant<
            std::monostate,
            bool,
            aldo_execmode,
            et::diff,
            aldo_haltexpr,
            int,
            probe>;

    template<std::convertible_to<payload> T = std::monostate>
    constexpr command_state(Command c, T v = {}) noexcept : cmd{c}, value{v} {}

    Command cmd;
    payload value;
};

struct viewstate {
    void addProbeCommand(aldo_interrupt signal, bool active)
    {
        commands.emplace(Command::probe, command_state::probe{signal, active});
    }

    std::queue<command_state> commands;
    RunClock clock;
    palette::sz colorSelection = 0;
    bool
        running = true, showAbout = false, showDemo = false,
        showDesignPalette = false;
};

}

#endif

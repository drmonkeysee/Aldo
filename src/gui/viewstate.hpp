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
#include <optional>
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
    halt,
    mode,
    openROM,
    paletteLoad,
    paletteUnload,
    probe,
    quit,
    resetVectorClear,
    resetVectorOverride,
    tprobe,
    zeroRamOnPowerup,
};

struct command_state {
    using probe = std::pair<aldo_probe, bool>;
    using tprobe = std::pair<aldo_probe, aldo_trivalue>;
    using payload =
        std::variant<
            std::monostate,
            bool,
            aldo_execmode,
            et::diff,
            aldo_haltexpr,
            int,
            probe,
            tprobe>;

    template<std::convertible_to<payload> T = std::monostate>
    constexpr command_state(Command c, T v = {}) noexcept : cmd{c}, value{v} {}

    Command cmd;
    payload value;
};

struct viewstate {
    void addProbeCommand(aldo_probe signal, bool active)
    {
        commands.emplace(Command::probe, command_state::probe{signal, active});
    }
    void addTprobeCommand(aldo_probe signal, aldo_trivalue val)
    {
        commands.emplace(Command::tprobe, command_state::tprobe{signal, val});
    }

    std::queue<command_state> commands;
    RunClock clock;
    palette::sz colorSelection = 0;
    std::optional<et::size> selectedInstruction;
    bool
        running = true, showAbout = false, showDemo = false,
        showDesignPalette = false;
};

}

#endif

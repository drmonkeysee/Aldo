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
#include "runclock.hpp"

#include <SDL2/SDL.h>

#include <concepts>
#include <queue>
#include <utility>
#include <variant>

namespace aldo
{

enum class Command {
    breakpointAdd,
    breakpointsClear,
    breakpointsExport,
    breakpointsOpen,
    breakpointRemove,
    breakpointToggle,
    halt,
    interrupt,
    mode,
    launchStudio,
    openROM,
    overrideReset,
    quit,
};

struct event {
    using interrupt = std::pair<csig_interrupt, bool>;
    using payload =
        std::variant<
            std::monostate,
            bool,
            csig_excmode,
            et::diff,
            haltexpr,
            int,
            interrupt>;

    template<std::convertible_to<payload> T = std::monostate>
    constexpr event(Command c, T v = {}) noexcept : cmd{c}, value{v} {}

    Command cmd;
    payload value;
};

struct viewstate {
    std::queue<event> events;
    runclock clock;
    struct {
        SDL_Point
            bounds{256, 240}, pos{bounds.x / 2, bounds.y / 2}, velocity{1, 1};
        int halfdim = 25;
    } bouncer;
    bool running = true, showAbout = false, showDemo = false;
};

}

#endif

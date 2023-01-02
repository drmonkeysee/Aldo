//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include "ctrlsignal.h"
#include "haltexpr.h"
#include "runclock.hpp"

#include <SDL2/SDL.h>

#include <queue>
#include <utility>
#include <variant>
#include <cstddef>

namespace aldo
{

enum class Command {
    breakpointAdd,
    breakpointsClear,
    breakpointRemove,
    breakpointToggle,
    halt,
    interrupt,
    mode,
    launchStudio,
    openFile,
    overrideReset,
    quit,
};

using interrupt_event = std::pair<csig_interrupt, bool>;

struct event {
    template<typename T = std::monostate>
    constexpr event(Command c, T v = {}) noexcept : cmd{c}, value{v} {}

    Command cmd;
    std::variant<
        std::monostate,
        bool,
        csig_excmode,
        haltexpr,
        int,
        interrupt_event,
        std::ptrdiff_t
    > value;
};

struct viewstate {
    std::queue<event> events;
    runclock clock;
    struct {
        SDL_Point
            bounds{256, 240}, pos{bounds.x / 2, bounds.y / 2}, velocity{1, 1};
        int halfdim = 25;
    } bouncer;
    bool running = true, showDemo = false;
};

}

#endif

//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include <SDL2/SDL.h>

#include <queue>
#include <variant>

namespace aldo
{

enum class Command {
    execMode,
    halt,
    quit,
    signalIRQ,
    signalNMI,
    signalReset,
};

struct event {
    constexpr event(Command c) : cmd{c} {}
    template<typename T>
    constexpr event(Command c, T v) noexcept : cmd{c}, value{v} {}

    Command cmd;
    std::variant<std::monostate, bool, csig_excmode> value;
};

struct viewstate {
    std::queue<event> events;
    struct {
        SDL_Point bounds, pos, velocity;
        int halfdim;
    } bouncer;
    bool running = true, showBouncer = true, showCpu = true, showDemo = false;
};

}

#endif

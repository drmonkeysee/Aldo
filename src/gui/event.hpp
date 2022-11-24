//
//  event.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/23/22.
//

#ifndef Aldo_gui_event_hpp
#define Aldo_gui_event_hpp

#include "ctrlsignal.h"
#include "nes.h"

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
    template<typename T>
    constexpr event(Command c, T v) noexcept : cmd{c}, value{v} {}

    Command cmd;
    std::variant<bool, csig_excmode> value;
};

void handle_events(std::queue<event>& events, nes* console);

}

#endif

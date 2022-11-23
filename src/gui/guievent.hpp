//
//  guievent.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/23/22.
//

#ifndef Aldo_gui_guievent_hpp
#define Aldo_gui_guievent_hpp

#include "ctrlsignal.h"
#include "nes.h"

#include <queue>
#include <variant>

namespace aldo
{

enum class Command {
    halt,
    mode,
    irq,
    nmi,
    res,
};

struct guievent {
    template<typename T>
    constexpr guievent(Command c, T v) noexcept : cmd{c}, value{v} {}

    Command cmd;
    std::variant<bool, csig_excmode> value;
};

void guievent_process(std::queue<guievent>& events, nes* console);

}

#endif

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

enum class command {
    Halt,
    Mode,
    Irq,
    Nmi,
    Res,
};

struct guievent {
    template<typename T>
    guievent(command c, T v) : cmd{c}, value{v} {}

    command cmd;
    std::variant<bool, csig_excmode> value;
};

void guievent_process(std::queue<guievent>& events, nes* console);

}

#endif

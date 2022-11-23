//
//  guievent.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/23/22.
//

#include "guievent.hpp"

namespace
{

auto process_event(const aldo::guievent& ev, nes* console)
{
    switch (ev.cmd) {
    case aldo::command::Halt:
        if (std::get<bool>(ev.value)) {
            nes_halt(console);
        } else {
            nes_ready(console);
        }
        break;
    case aldo::command::Mode:
        nes_mode(console, std::get<csig_excmode>(ev.value));
        break;
    case aldo::command::Irq:
        if (std::get<bool>(ev.value)) {
            nes_interrupt(console, CSGI_IRQ);
        } else {
            nes_clear(console, CSGI_IRQ);
        }
        break;
    case aldo::command::Nmi:
        if (std::get<bool>(ev.value)) {
            nes_interrupt(console, CSGI_NMI);
        } else {
            nes_clear(console, CSGI_NMI);
        }
        break;
    case aldo::command::Res:
        if (std::get<bool>(ev.value)) {
            nes_interrupt(console, CSGI_RES);
        } else {
            nes_clear(console, CSGI_RES);
        }
        break;
    }
}

}

void aldo::guievent_process(std::queue<aldo::guievent>& events, nes* console)
{
    while (!events.empty()) {
        const auto& ev = events.front();
        process_event(ev, console);
        events.pop();
    }
}

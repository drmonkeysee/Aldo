//
//  event.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/23/22.
//

#include "event.hpp"

#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace
{

auto invalid_command(aldo::Command c)
{
    std::stringstream s;
    s
        << "Invalid gui event command ("
        << static_cast<std::underlying_type_t<aldo::Command>>(c)
        << ')';
    return s.str();
}

auto process_event(const aldo::event& ev, nes* console)
{
    switch (ev.cmd) {
    case aldo::Command::halt:
        if (std::get<bool>(ev.value)) {
            nes_halt(console);
        } else {
            nes_ready(console);
        }
        break;
    case aldo::Command::mode:
        nes_mode(console, std::get<csig_excmode>(ev.value));
        break;
    case aldo::Command::irq:
        if (std::get<bool>(ev.value)) {
            nes_interrupt(console, CSGI_IRQ);
        } else {
            nes_clear(console, CSGI_IRQ);
        }
        break;
    case aldo::Command::nmi:
        if (std::get<bool>(ev.value)) {
            nes_interrupt(console, CSGI_NMI);
        } else {
            nes_clear(console, CSGI_NMI);
        }
        break;
    case aldo::Command::res:
        if (std::get<bool>(ev.value)) {
            nes_interrupt(console, CSGI_RES);
        } else {
            nes_clear(console, CSGI_RES);
        }
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}

}

void aldo::handle_events(std::queue<aldo::event>& events, nes* console)
{
    while (!events.empty()) {
        const auto& ev = events.front();
        process_event(ev, console);
        events.pop();
    }
}

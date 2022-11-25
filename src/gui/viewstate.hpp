//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include "ctrlsignal.h"
#include "cycleclock.h"
#include "tsutil.h"

#include <SDL2/SDL.h>

#include <queue>
#include <utility>
#include <variant>
#include <ctime>

namespace aldo
{

enum class Command {
    halt,
    interrupt,
    mode,
    quit,
};

using interrupt_event = std::pair<csig_interrupt, bool>;

struct event {
    constexpr event(Command c) noexcept : cmd{c} {}
    template<typename T>
    constexpr event(Command c, T v) noexcept : cmd{c}, value{v} {}

    Command cmd;
    std::variant<
        std::monostate,
        bool,
        csig_excmode,
        interrupt_event
    > value;
};

struct runclock {
    void start()
    {
        cycleclock_start(&cyclock);
    }

    void tick_start(const struct console_state* snapshot)
    {
        cycleclock_tickstart(&cyclock, !snapshot->lines.ready);
    }

    void emutime()
    {
        const std::timespec elapsed = timespec_elapsed(&cyclock.current);
        emutime_ms = timespec_to_ms(&elapsed);
    }

    void tick_end()
    {
        cycleclock_tickend(&cyclock);
    }

    cycleclock cyclock{.cycles_per_sec = 4};
    double emutime_ms = 0;
};

struct viewstate {
    std::queue<event> events;
    runclock clock;
    struct {
        SDL_Point bounds, pos, velocity;
        int halfdim;
    } bouncer;
    bool running = true, showBouncer = true, showCpu = true, showDemo = false;
};

}

#endif

//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include "snapshot.h"

#include <SDL2/SDL.h>

namespace aldo
{

struct viewstate {
    struct {
        SDL_Point bounds, pos, velocity;
        int halfdim;
    } bouncer;
    const struct console_state& snapshot;
    bool running = true, showBouncer = true, showCpu = true, showDemo = false;
};

}

#endif

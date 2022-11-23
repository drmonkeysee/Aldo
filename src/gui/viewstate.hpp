//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include "guievent.hpp"

#include <SDL2/SDL.h>

#include <queue>

namespace aldo
{

struct viewstate {
    std::queue<guievent> guiEvents;
    struct {
        SDL_Point bounds, pos, velocity;
        int halfdim;
    } bouncer;
    bool running = true, showBouncer = true, showCpu = true, showDemo = false;
};

}

#endif

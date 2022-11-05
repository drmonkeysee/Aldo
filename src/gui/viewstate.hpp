//
//  viewstate.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_viewstate_hpp
#define Aldo_gui_viewstate_hpp

#include <SDL2/SDL.h>

namespace aldo
{

struct viewstate {
    struct {
        SDL_Point bounds, pos, velocity;
        int halfdim;
    } bouncer;
    bool running, showDemo;
};

}

#endif

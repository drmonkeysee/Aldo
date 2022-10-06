//
//  guiplatform.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#ifndef Aldo_gui_guiplatform_h
#define Aldo_gui_guiplatform_h

#include <SDL2/SDL.h>

#include <stdbool.h>

struct gui_platform {
    bool (*is_hidpi)(void);
    float (*render_scale_factor)(SDL_Window *);
};

bool gui_platform_init(struct gui_platform *platform);

#endif

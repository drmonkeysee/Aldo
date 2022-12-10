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
#include <stddef.h>

#include "bridgeopen.h"
struct gui_platform {
    bool (*appname)(size_t, char[br_noalias], size_t *);
    bool (*is_hidpi)(void);
    float (*render_scale_factor)(SDL_Window *);
    bool (*open_file)(size_t, char[br_noalias], size_t *);
    bool (*display_error)(const char *, const char *);
};

br_checkerror
bool gui_platform_init(struct gui_platform *platform) br_nothrow;
#include "bridgeclose.h"

#endif

//
//  guiplatform.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#ifndef Aldo_gui_guiplatform_h
#define Aldo_gui_guiplatform_h

#include <stdbool.h>

#include "bridgeopen.h"
struct gui_platform {
    br_ownresult
    char *(*appname)(void) br_nothrow;
    bool (*is_hidpi)(void) br_nothrow;
    float (*render_scale_factor)(void *) br_nothrow;
    br_ownresult
    char *(*open_file)(const char*, const char* const[]) br_nothrow;
    void (*launch_studio)(void) br_nothrow;
    bool (*display_error)(const char *, const char *) br_nothrow;
    void (*free_buffer)(char *) br_nothrow;
};
#include "bridgeclose.h"

#endif

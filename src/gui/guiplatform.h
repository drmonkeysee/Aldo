//
//  guiplatform.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#ifndef Aldo_gui_guiplatform_h
#define Aldo_gui_guiplatform_h

#include "bridgeopen.h"
struct gui_platform {
    char *(*appname)() aldo_nothrow;
    char *(*orgname)() aldo_nothrow;
    bool (*is_hidpi)() aldo_nothrow;
    float (*render_scale_factor)(void *) aldo_nothrow;
    char *(*aldo_pcallc open_file)(const char*,
                                   const char* const[]) aldo_nothrow;
    char *(*aldo_pcallc save_file)(const char*, const char*) aldo_nothrow;
    void (*free_buffer)(char *) aldo_nothrow;
};
#include "bridgeclose.h"

#endif

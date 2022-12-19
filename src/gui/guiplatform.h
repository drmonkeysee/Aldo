//
//  guiplatform.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#ifndef Aldo_gui_guiplatform_h
#define Aldo_gui_guiplatform_h

#include <stdbool.h>

struct gui_platform {
    char *(*appname)(void);
    bool (*is_hidpi)(void);
    float (*render_scale_factor)(void *);
    char *(*open_file)(void);
    void (*activate_cart_inspector)(void *);
    bool (*display_error)(const char *, const char *);
    void (*free_buffer)(char *);
    void (*cleanup)(void **ctx);
    void *ctx;
};

#endif

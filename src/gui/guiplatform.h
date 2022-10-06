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
    bool (*is_hidpi)(void);
};

bool gui_platform_init(struct gui_platform *platform);

#endif

//
//  uisdl.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/11/22.
//

#include <assert.h>
#include <stddef.h>

#include "ui.h"

#include "sdlimgui.hpp"

int ui_sdl_init(struct ui_interface *ui)
{
    assert(ui != NULL);

    return sdlimgui_init(ui);
}

//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

struct ui_interface ui_batch_init(void);
struct ui_interface ui_ncurses_init(void);

struct ui_interface ui_init(void)
{
    return ui_ncurses_init();
}

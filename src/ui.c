//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

int ui_batch_init(struct ui_interface *ui);
int ui_ncurses_init(struct ui_interface *ui);

int ui_init(struct ui_interface *ui)
{
    //return ui_ncurses_init();
    return ui_batch_init(ui);
}

const char *ui_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        UI_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

#include "tsutil.h"

#include <time.h>

extern inline double timespec_to_ms(const struct timespec *);
extern inline struct timespec timespec_elapsed(const struct timespec *);

int ui_batch_init(struct ui_interface *ui);
int ui_ncurses_init(struct ui_interface *ui);

int ui_init(const struct control *appstate, struct ui_interface *ui)
{
    return appstate->batch ? ui_batch_init(ui) : ui_ncurses_init(ui);
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

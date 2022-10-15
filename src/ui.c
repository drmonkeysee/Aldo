//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

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

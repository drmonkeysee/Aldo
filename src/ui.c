//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

const char *aldo_ui_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case ALDO_##s: return e;
        ALDO_UIERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

//
//  ui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_ui_h
#define Aldo_ui_h

// X(symbol, value, error string)
#define UI_ERRCODE_X \
X(UI_ERR_ERNO, -1, "SYSTEM ERROR") \
X(UI_ERR_LIBINIT, -2, "LIBRARY INIT FAILURE") \
X(UI_ERR_EXCEPTION, -3, "PROGRAM EXCEPTION")

enum {
#define X(s, v, e) s = v,
    UI_ERRCODE_X
#undef X
};

#include "bridgeopen.h"
br_libexport
const char *ui_errstr(int err) br_nothrow;
#include "bridgeclose.h"

#endif

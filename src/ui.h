//
//  ui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_ui_h
#define Aldo_ui_h

// X(symbol, value, error string)
#define ALDO_UIERRCODE_X \
X(UIERR_ERNO, -1, "SYSTEM ERROR") \
X(UIERR_LIBINIT, -2, "LIBRARY INIT FAILURE") \
X(UIERR_EXCEPTION, -3, "PROGRAM EXCEPTION")

enum {
#define X(s, v, e) ALDO_##s = v,
    ALDO_UIERRCODE_X
#undef X
};

#include "bridgeopen.h"
br_libexport
const char *aldo_ui_errstr(int err) br_nothrow;
#include "bridgeclose.h"

#endif

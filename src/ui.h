//
//  ui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_ui_h
#define Aldo_ui_h

#include "nes.h"
#include "snapshot.h"

// X(symbol, value, error string)
#define UI_ERRCODE_X \
X(UI_ERR_ERNO, -1, "SYSTEM ERROR") \
X(UI_ERR_LIBINIT, -2, "LIBRARY INIT FAILURE")

enum {
#define X(s, v, e) s = v,
    UI_ERRCODE_X
#undef X
};

#include "bridgeopen.h"
typedef void ui_loop(nes *, struct console_state *) br_nothrow;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *ui_errstr(int err) br_nothrow;
#include "bridgeclose.h"

#endif

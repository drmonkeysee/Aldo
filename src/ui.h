//
//  ui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_ui_h
#define Aldo_ui_h

#include "cliargs.h"
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
typedef void ui_loop(nes *, struct console_state *) aldo_nothrow;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *ui_errstr(int err) aldo_nothrow;

// NOTE: common batch mode for CLI and GUI mode
int ui_batch_init(const struct cliargs *args, ui_loop **loop) aldo_nothrow;
#include "bridgeclose.h"

#endif

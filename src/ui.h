//
//  ui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_ui_h
#define Aldo_ui_h

#include "control.h"
#include "nes.h"
#include "snapshot.h"

#include "bridgeopen.h"

// X(symbol, value, error string)
#define UI_ERRCODE_X \
X(UI_ERR_ERNO, -1, "SYSTEM ERROR") \
X(UI_ERR_LIBINIT, -2, "LIBRARY INIT FAILURE")

enum {
#define X(s, v, e) s = v,
    UI_ERRCODE_X
#undef X
};

typedef void ui_loop(struct control *, nes *, struct console_state *) aldo_noexcept;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *ui_errstr(int err) aldo_noexcept;

// NOTE: common batch mode for CLI and GUI mode
int ui_batch_init(ui_loop **loop) aldo_noexcept;

#include "bridgeclose.h"
#endif

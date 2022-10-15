//
//  ui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_ui_h
#define Aldo_ui_h

#include "control.h"
#include "snapshot.h"

// X(symbol, value, error string)
#define UI_ERRCODE_X \
X(UI_ERR_ERNO, -1, "SYSTEM ERROR")

enum {
#define X(s, v, e) s = v,
    UI_ERRCODE_X
#undef X
};

struct ui_interface {
    void (*tick_start)(struct control *appstate,
                       const struct console_state *snapshot);
    void (*tick_end)(struct control *appstate);
    int (*pollinput)(void);
    void (*render)(const struct control *appstate,
                   const struct console_state *snapshot);
    void (*cleanup)(const struct control *appstate,
                    const struct console_state *snapshot);
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *ui_errstr(int err);

// NOTE: common batch mode for CLI and GUI mode
int ui_batch_init(struct ui_interface *ui);

#endif

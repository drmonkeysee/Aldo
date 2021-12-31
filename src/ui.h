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

struct ui_interface {
    void (*tick_start)(struct control *appstate,
                       const struct console_state *snapshot);
    void (*tick_end)(void);
    int (*pollinput)(void);
    void (*refresh)(const struct control *appstate,
                    const struct console_state *snapshot);
    void (*cleanup)(void);
};

struct ui_interface ui_init(void);

#endif

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

void ui_init(void);
void ui_cleanup(void);

void ui_tick_start(struct control *appstate,
                   const struct console_state *snapshot);
void ui_tick_end(void);

int ui_pollinput(void);
void ui_refresh(const struct control *appstate,
                const struct console_state *snapshot);

#endif

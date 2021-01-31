//
//  devui.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#ifndef Aldo_devui_h
#define Aldo_devui_h

#include "emu/snapshot.h"

#include <stdint.h>

extern int CurrentRamViewPage;

void ui_init(void);
void ui_cleanup(void);

int ui_pollinput(void);
void ui_ram_next(void);
void ui_ram_prev(void);

void ui_refresh(const struct console_state *snapshot, uint64_t cycles);

#endif

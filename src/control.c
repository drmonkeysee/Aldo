//
//  control.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/14/21.
//

#include "control.h"

const int RamSheets = 4;

extern inline void ctl_toggle_excmode(struct control *),
                   ctl_ram_next(struct control *),
                   ctl_ram_prev(struct control *);

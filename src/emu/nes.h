//
//  nes.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#ifndef Aldo_emu_nes_h
#define Aldo_emu_nes_h

int nes_rand(void);

typedef struct nes_console nes;

nes *nes_new(void);
void nes_free(nes *self);

#endif

//
//  ppuhelp.h
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 7/7/24.
//

#ifndef AldoTests_ppuhelp_h
#define AldoTests_ppuhelp_h

#include "bus.h"
#include "ppu.h"

#include <stdint.h>

#define ppt_get_ppu(ctx) &((struct ppu_test_context *)(ctx))->ppu
#define ppt_get_mbus(ctx) ((struct ppu_test_context *)(ctx))->mbus

struct ppu_test_context {
    struct aldo_rp2c02 ppu;
    bus *mbus, *vbus;
};

extern uint8_t VRam[];

void ppu_setup(void **ctx);
void ppu_teardown(void **ctx);

#endif

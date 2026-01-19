//
//  ppuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 7/7/24.
//

#include "ppuhelp.h"

#include "bytes.h"
#include "ctrlsignal.h"

#include <stdlib.h>

static bool test_vread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    if (ALDO_MEMBLOCK_8KB <= addr && addr < ALDO_MEMBLOCK_16KB) {
        *d = ((uint8_t *)ctx)[addr & 0x3];
        return true;
    }
    return false;
}

static bool test_vwrite(void *ctx, uint16_t addr, uint8_t d)
{
    if (ALDO_MEMBLOCK_8KB <= addr && addr < 0x3f00) {
        ((uint8_t *)ctx)[addr & 0x3] = d;
        return true;
    }
    return false;
}

//
// MARK: - Public Interface
//

uint8_t VRam[4];

void ppu_setup(void **ctx)
{
    struct ppu_test_context *c = calloc(1, sizeof *c);
    // enough main bus to map $2000 - $3FFF for ppu registers
    c->mbus = aldo_bus_new(ALDO_BITWIDTH_16KB, 2, ALDO_MEMBLOCK_8KB);
    VRam[0] = 0x11;
    VRam[1] = 0x22;
    VRam[2] = 0x33;
    VRam[3] = 0x44;
    c->ppu.vbus = c->vbus = aldo_bus_new(ALDO_BITWIDTH_16KB, 2,
                                         ALDO_MEMBLOCK_8KB);
    aldo_bus_set(c->vbus, ALDO_MEMBLOCK_8KB, (struct aldo_busdevice){
        .read = test_vread,
        .write = test_vwrite,
        .ctx = VRam,
    });
    aldo_ppu_connect(&c->ppu, c->mbus);
    // Run powerup and reset sequence and then force internal state
    // to a known zero-value.
    aldo_ppu_powerup(&c->ppu);
    aldo_ppu_cycle(&c->ppu);
    c->ppu.line = c->ppu.dot = 0;
    c->ppu.rst = ALDO_SIG_CLEAR;
    *ctx = c;
}

void ppu_teardown(void **ctx)
{
    struct ppu_test_context *c = *ctx;
    aldo_bus_free(c->vbus);
    aldo_bus_free(c->mbus);
    free(c);
}

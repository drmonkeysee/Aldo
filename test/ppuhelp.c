//
//  ppuhelp.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 7/7/24.
//

#include "ppuhelp.h"

#include "bytes.h"
#include "ctrlsignal.h"

#include <stdbool.h>
#include <stdlib.h>

static bool test_vread(void *restrict ctx, uint16_t addr, uint8_t *restrict d)
{
    if (MEMBLOCK_8KB <= addr && addr < MEMBLOCK_16KB) {
        *d = ((uint8_t *)ctx)[addr & 0x3];
        return true;
    }
    return false;
}

static bool test_vwrite(void *ctx, uint16_t addr, uint8_t d)
{
    if (MEMBLOCK_8KB <= addr && addr < 0x3f00) {
        ((uint8_t *)ctx)[addr & 0x3] = d;
        return true;
    }
    return false;
}

#pragma mark - Public Interface
//
// Public Interface
//

uint8_t VRam[4];

void ppu_setup(void **ctx)
{
    struct ppu_test_context *const c = calloc(sizeof *c, 1);
    // NOTE: enough main bus to map $2000 - $3FFF for ppu registers
    c->mbus = bus_new(BITWIDTH_16KB, 2, MEMBLOCK_8KB);
    VRam[0] = 0x11;
    VRam[1] = 0x22;
    VRam[2] = 0x33;
    VRam[3] = 0x44;
    c->ppu.vbus = c->vbus = bus_new(BITWIDTH_16KB, 1);
    bus_set(c->vbus, 0, (struct busdevice){
        .read = test_vread,
        .write = test_vwrite,
        .ctx = VRam,
    });
    ppu_connect(&c->ppu, c->mbus);
    // NOTE: run powerup and reset sequence and then force internal state to a
    // known zero-value.
    ppu_powerup(&c->ppu);
    ppu_cycle(&c->ppu);
    c->ppu.line = c->ppu.dot = 0;
    c->ppu.rst = CSGS_CLEAR;
    *ctx = c;
}

void ppu_teardown(void **ctx)
{
    struct ppu_test_context *const c = *ctx;
    bus_free(c->vbus);
    bus_free(c->mbus);
    free(c);
}

//
//  ppu.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ppu.h"

#include <assert.h>
#include <stddef.h>

//
// Main Bus Device (PPU registers)
//

static bool reg_read(const void *restrict ctx, uint16_t addr,
                     uint8_t *restrict d)
{
    // TODO: get correct register
    (void)addr;
    *d = ((struct rp2c02 *)ctx)->regd;
    return true;
}

static bool reg_write(void *ctx, uint16_t addr, uint8_t d)
{
    // TODO: get correct register
    (void)addr;
    ((struct rp2c02 *)ctx)->regd = d;
    return true;
}

//
// Public Interface
//

void ppu_connect(struct rp2c02 *self, void *restrict vram, bus *mbus)
{
    assert(self != NULL);
    assert(vram != NULL);
    assert(mbus != NULL);

    self->mbus = mbus;
    const bool r = bus_set(self->mbus, MEMBLOCK_8KB, (struct busdevice){
        .read = reg_read,
        .write = reg_write,
        .ctx = self,
    });
    assert(r);
    // TODO: partition this properly
    self->vbus = bus_new(BITWIDTH_16KB, 1);
    // TODO: add vbus device
}

void ppu_disconnect(struct rp2c02 *self)
{
    assert(self != NULL);

    bus_free(self->vbus);
    self->mbus = self->vbus = NULL;
}

void ppu_powerup(struct rp2c02 *self)
{
    assert(self != NULL);
    assert(self->mbus != NULL);
    assert(self->vbus != NULL);

    self->dot = self->line = self->regd = 0;
    self->odd = self->w = false;
}

int ppu_cycle(struct rp2c02 *self)
{
    assert(self != NULL);

    // NOTE: a single frame is 262 scanlines of 341 dots,
    // counting h-blank, v-blank, overscan, etc.
    static const int dots = 341, lines = 262;

    if (++self->dot >= dots) {
        self->dot = 0;
        if (++self->line >= lines) {
            self->line = 0;
        }
    }

    return 1;
}

void ppu_snapshot(const struct rp2c02 *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->ppu.dot = self->dot;
    snapshot->ppu.line = self->line;
    snapshot->ppu.register_databus = self->regd;
}

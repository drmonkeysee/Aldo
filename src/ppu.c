//
//  ppu.c
//  Aldo
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ppu.h"

#include <assert.h>
#include <stddef.h>

// NOTE: a single NTSC frame is 262 scanlines of 341 dots, counting h-blank,
// v-blank, overscan, etc; nominally 1 frame is 262 * 341 = 89342 ppu cycles;
// however with rendering enabled the odd frames skip a dot for an overall
// average cycle count of 89341.5 per frame.
static const int
    Dots = 341, Lines = 262,
// NOTE: NTSC CPU:PPU cycle ratio
    CycleRatio = 3;

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

static int cycle(struct rp2c02 *self)
{
    if (++self->dot >= Dots) {
        self->dot = 0;
        if (++self->line >= Lines) {
            self->line = 0;
        }
    }
    return 1;
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

int ppu_cycle(struct rp2c02 *self, int cpu_cycles)
{
    assert(self != NULL);

    const int total = cpu_cycles * CycleRatio;
    int cycles = 0;
    for (int i = 0; i < total; ++i, cycles += cycle(self));
    return cycles;
}

void ppu_snapshot(const struct rp2c02 *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->ppu.dot = self->dot;
    snapshot->ppu.line = self->line;
    snapshot->ppu.register_databus = self->regd;
}

struct ppu_coord ppu_pixel_trace(const struct rp2c02 *self, int adjustment)
{
    struct ppu_coord pixel = {
        self->dot + (adjustment * CycleRatio),
        self->line,
    };
    if (pixel.dot < 0) {
        pixel.dot += Dots;
        if (--pixel.line < 0) {
            pixel.line += Lines;
        }
    }
    return pixel;
}

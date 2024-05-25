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

//
// Internal Operations
//

static void reset(struct rp2c02 *self)
{
    self->dot = self->line = 0;
    self->odd = self->w = false;
    // TODO: clear this later when vblank and sprite 0 are cleared
    // (use SERVICED?)
    self->res = CSGS_CLEAR;
}

// NOTE: this follows the same sequence as CPU reset sequence and is checked
// only on CPU-cycle boundaries rather than each PPU cycle; this keeps the two
// chips in sync when handling the RESET signal.
static void handle_reset(struct rp2c02 *self)
{
    // NOTE: reset should never be in serviced state
    assert(self->res != CSGS_SERVICED);

    if (self->res == CSGS_PENDING) {
        self->res = CSGS_COMMITTED;
    }

    if (self->signal.res) {
        if (self->res == CSGS_DETECTED) {
            self->res = CSGS_CLEAR;
        } else if (self->res == CSGS_COMMITTED) {
            // NOTE: reset is committed and signal is no longer pulled low
            reset(self);
        }
    } else {
        switch (self->res) {
        case CSGS_CLEAR:
            self->res = CSGS_DETECTED;
            break;
        case CSGS_DETECTED:
            self->res = CSGS_PENDING;
            break;
        default:
            // NOTE: as long as reset line is held low there is no further
            // effect on PPU execution.
            break;
        }
    }
}

static int cycle(struct rp2c02 *self)
{
    // TODO: do dot advancement last, leave PPU on next dot to be drawn;
    // analogous to stack pointer always pointing at next byte to be written.
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

    self->signal.res = true;

    // NOTE: initialize ppu to known startup state; anything affected by the
    // reset sequence is deferred until that phase.
    self->regd = 0;

    // NOTE: simulate res set on startup to engage reset sequence
    self->res = CSGS_PENDING;
}

int ppu_cycle(struct rp2c02 *self, int cpu_cycles)
{
    assert(self != NULL);

    // NOTE: for simplicity, handle RESET signal on CPU cycle boundaries
    handle_reset(self);

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

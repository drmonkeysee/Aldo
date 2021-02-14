//
//  cpu.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#include "cpu.h"

#include "bytes.h"
#include "traits.h"

#include <assert.h>
#include <stddef.h>

static bool read(struct mos6502 *self)
{
    if (self->addrbus <= CpuRamMaxAddr) {
        self->databus = self->ram[self->addrbus & CpuRamAddrMask];
        return true;
    }
    if (CpuCartMinAddr <= self->addrbus && self->addrbus <= CpuCartMaxAddr) {
        self->databus = self->cart[self->addrbus & CpuCartAddrMask];
        return true;
    }
    return false;
}

static void setpc(struct mos6502 *self, uint8_t lo, uint8_t hi)
{
    self->pc = bytowr(lo, hi);
}

static uint8_t get_p(const struct mos6502 *self)
{
    uint8_t p = 0;
    p |= self->p.c << 0;
    p |= self->p.z << 1;
    p |= self->p.i << 2;
    p |= self->p.d << 3;
    p |= self->p.b << 4;
    p |= self->p.u << 5;
    p |= self->p.v << 6;
    p |= self->p.n << 7;
    return p;
}

static void set_p(struct mos6502 *self, uint8_t p)
{
    self->p.c = p & 0x1;
    self->p.z = p & 0x2;
    self->p.i = p & 0x4;
    self->p.d = p & 0x8;
    self->p.b = p & 0x10;
    self->p.u = p & 0x20;
    self->p.v = p & 0x40;
    self->p.n = p & 0x80;
}

//
// Public Interface
//

void cpu_powerup(struct mos6502 *self)
{
    assert(self != NULL);

    // NOTE: set B, I, and unused flags high
    set_p(self, 0x34);
    self->a = self->x = self->y = self->s = 0;

    // NOTE: Interrupts are inverted, high means no interrupt; rw high is read
    self->signal.irq = self->signal.nmi = self->signal.res = self->signal.rw
        = true;
    self->signal.rdy = self->signal.sync = false;

    // NOTE: all other cpu elements are indeterminate on powerup
}

void cpu_reset(struct mos6502 *self)
{
    // TODO: this will eventually set a signal
    // and execute an instruction sequence
    assert(self != NULL);

    // TODO: reset sequence begins after reset signal goes from low to high
    self->signal.res = true;

    // TODO: fake the execution of the RES sequence and load relevant
    // data into the datapath.
    self->t = MaxCycleCount;
    self->addrbus = ResetVector;
    read(self);
    self->adl = self->databus;
    self->addrbus = ResetVector + 1;
    read(self);
    setpc(self, self->adl, self->databus);

    self->p.i = true;
    // NOTE: Reset runs through same sequence as BRK/IRQ
    // so the cpu does 3 phantom stack pushes;
    // on powerup this would result in $00 - $3 = $FD.
    self->s -= 3;

    self->end = true;
}

int cpu_clock(struct mos6502 *self, int maxcycles)
{
    assert(self != NULL);

    if (!self->signal.rdy) return 0;

    if (self->end) {
        self->end = false;
        // set t to -1 on new instruction;
        // auto-increment will start current cycle on T0.
        self->t = -1;
    }

    int cycles = 0;
    while (cycles < maxcycles) {
        switch (++self->t) {
        case 0:
            // NOTE: T0 is always an opcode fetch
            self->signal.sync = self->signal.rw = true;
            self->addrbus = self->pc++;
            read(self);
            self->opc = self->databus;
            ++cycles;
        }
    }
    return cycles;
}

void cpu_snapshot(const struct mos6502 *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->cpu.program_counter = self->pc;
    snapshot->cpu.accumulator = self->a;
    snapshot->cpu.stack_pointer = self->s;
    snapshot->cpu.status = get_p(self);
    snapshot->cpu.xindex = self->x;
    snapshot->cpu.yindex = self->y;

    snapshot->cpu.addressbus = self->addrbus;
    if (self->signal.sync) {
        snapshot->cpu.instaddr = self->addrbus;
    }
    snapshot->cpu.addrlow_latch = self->adl;
    snapshot->cpu.databus = self->databus;
    snapshot->cpu.exec_cycle = self->t;
    snapshot->cpu.opcode = self->opc;

    snapshot->lines.irq = self->signal.irq;
    snapshot->lines.nmi = self->signal.nmi;
    snapshot->lines.readwrite = self->signal.rw;
    snapshot->lines.ready = self->signal.rdy;
    snapshot->lines.reset = self->signal.res;
    snapshot->lines.sync = self->signal.sync;
}

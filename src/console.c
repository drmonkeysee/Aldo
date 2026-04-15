//
//  console.c
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#include "consoledef.h"

#include "aldo8.h"
#include "bus.h"
#include "bytes.h"
#include "cycleclock.h"
#include "nes.h"
#include "snapshot.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static const char *typename(enum aldo_console_type t)
{
    switch (t) {
#define X(s, n) case ALDO_##s: return n;
        ALDO_CONSOLE_TYPE_X
#undef X
    default:
        return "UNKNOWN CONSOLE TYPE";
    }
}

//
// MARK: System Clocking

static void set_cpu_pins(struct aldo_console_base *self)
{
    self->cpu.signal.rdy = self->probe.rdy;
    // interrupt lines are active low
    self->cpu.signal.irq = !self->probe.irq;
    self->cpu.signal.nmi = !self->probe.nmi;
    self->cpu.signal.rst = !self->probe.rst;
}

static bool clock_cpu(struct aldo_console_base *self, struct aldo_clock *clock)
{
    auto cycles = aldo_cpu_cycle(&self->cpu);
    set_cpu_pins(self);
    clock->cycles += (uint64_t)cycles;
    // TODO: trace
    //instruction_trace(self, clock, -cycles);
    return true;
}

static void clock_system(struct aldo_console_base *self, struct aldo_clock *clock)
{
    bool cputick = self->type == ALDO_CONSOLE_NES
                    ? aldo_nes_clock((aldo_nes *)self, clock)
                    : clock_cpu(self, clock);

    switch (self->mode) {
    case ALDO_EXC_CYCLE:
        // TODO: what should these settings do in frame mode?
        aldo_console_halt(self, cputick);
        break;
    case ALDO_EXC_STEP:
        if (clock->rate_factor == aldo_console_cycle_factor(self)) {
            aldo_console_halt(self, cputick && self->cpu.signal.sync);
        }
        break;
    case ALDO_EXC_RUN:
    default:
        break;
    }

    if (aldo_debug_break(self->dbg, clock)) {
        aldo_console_halt(self, true);
    }
}

//
// MARK: Snapshotting
//

static void reset_snapshot(struct aldo_console_base *self)
{
    if (!self->snp) return;

    assert(self->snp->video != nullptr);
    self->snp->video->newframe = false;
}

static void snapshot_core(struct aldo_console_base *self)
{
    auto snp = self->snp;
    if (!snp) return;

    aldo_cpu_snapshot(&self->cpu, self->snp);
    aldo_bus_copy(self->cpu.mbus, ALDO_CPU_VECTOR_NMI,
                  aldo_arrsz(snp->prg.vectors), snp->prg.vectors);

    auto prg = &snp->prg;
    assert(prg->curr != nullptr);
    prg->curr->length = aldo_bus_copy(self->cpu.mbus,
                                      snp->cpu.datapath.current_instruction,
                                      aldo_arrsz(prg->curr->pc), prg->curr->pc);
    switch (self->type) {
    case ALDO_CONSOLE_ALDO8:
        aldo_aldo8_snapshot_core((aldo_aldo8 *)self, snp);
        break;
    case ALDO_CONSOLE_NES:
        aldo_nes_snapshot_core((aldo_nes *)self, snp);
        break;
    default:
        assert(((void)"INVALID CONSOLE TYPE", false));
        break;
    }
}

static void init_snapshot(struct aldo_console_base *self)
{
    if (!self->snp) return;

    snapshot_core(self);
    if (self->type == ALDO_CONSOLE_NES) {
        aldo_nes_snapshot_init((aldo_nes *)self, self->snp);
    }
}

//
// MARK: Init/Cleanup
//

static void connect_cart(struct aldo_console_base *self, aldo_cart *c)
{
    self->cart = c;
    auto r = aldo_cart_mbus_connect(self->cart, self->cpu.mbus);
    (void)r, assert(r);
    if (self->conn) {
        self->conn(self);
    }
    aldo_debug_sync_bus(self->dbg);
}

static void disconnect_cart(struct aldo_console_base *self)
{
    // Debugger may have been attached to a cart-less CPU bus so reset
    // debugger even if there is no existing cart.
    aldo_debug_reset(self->dbg);
    if (!self->cart) return;
    if (self->dconn) {
        self->dconn(self);
    }
    aldo_cart_mbus_disconnect(self->cart, self->cpu.mbus);
    self->cart = nullptr;
}

static void setup(struct aldo_console_base *self, aldo_debugger *dbg,
                  FILE *tracelog)
{
    assert(self != nullptr);
    assert(dbg != nullptr);

    self->dconn = nullptr;
    self->dtor = nullptr;
    self->cart = nullptr;
    self->dbg = dbg;
    self->tracelog = tracelog;
    self->halted = self->cpu.bcd = self->probe.rdy = true;
    self->tracefailed = self->probe.irq = self->probe.nmi = self->probe.rst = false;
    self->params = (typeof(self->params)){.cyclefactor = 1};
    aldo_debug_cpu_connect(self->dbg, &self->cpu);
}

static void teardown(struct aldo_console_base *self)
{
    disconnect_cart(self);
    aldo_debug_cpu_disconnect(self->dbg);

    if (self->dtor) {
        self->dtor(self);
    }
    aldo_bus_free(self->cpu.mbus);
}

static aldo_console *new_aldo8(aldo_debugger *dbg, FILE *tracelog)
{
    aldo_console *c = malloc(Aldo_Aldo8Size);
    if (!c) return nullptr;

    setup(c, dbg, tracelog);
    c->type = ALDO_CONSOLE_ALDO8;

    if (aldo_aldo8_init((aldo_aldo8 *)c)) {
        return c;
    } else {
        aldo_console_free(c);
        return nullptr;
    }
}

static aldo_console *new_nes(aldo_debugger *dbg, FILE *tracelog)
{
    aldo_console *c = malloc(Aldo_NesSize);
    if (!c) return nullptr;

    setup(c, dbg, tracelog);
    c->type = ALDO_CONSOLE_NES;
    c->conn = aldo_nes_cart_connect;
    c->dconn = aldo_nes_cart_disconnect;
    c->dtor = aldo_nes_cleanup;

    if (aldo_nes_init((aldo_nes *)c)) {
        return c;
    } else {
        aldo_console_free(c);
        return nullptr;
    }
}

static bool swap_console(enum aldo_console_type t, enum aldo_cartformat f)
{
    switch (t) {
    case ALDO_CONSOLE_NES:
        return f != ALDO_CRTF_INES;
    case ALDO_CONSOLE_ALDO8:
        return f != ALDO_CRTF_RAW;
    default:
        assert(((void)"INVALID CONSOLE TYPE", false));
        return true;
    }
}

//
// MARK: - Public Interface
//

bool aldo_console_poweron(aldo_console **cn, aldo_cart *c, aldo_debugger *dbg,
                          struct aldo_snapshot *snp, FILE *tracelog, bool zeroram)
{
    assert(cn != nullptr);
    assert(c != nullptr);
    assert(dbg != nullptr);
    assert(snp != nullptr);

    struct aldo_cartinfo info;
    aldo_cart_getinfo(c, &info);
    aldo_console *self = *cn;
    // TODO: add snapshot before powerup, add init snapshot to power up
    if (!self || swap_console(self->type, info.format)) {
        aldo_console *newcn = info.format == ALDO_CRTF_INES
                                ? new_nes(dbg, tracelog)
                                : new_aldo8(dbg, tracelog);
        if (!newcn) return false;

        if (self) {
            aldo_console_powerdown(self);
            aldo_console_free(self);
        }
        self = newcn;
    } else {
        aldo_console_powerdown(self);
    }
    aldo_console_powerup(self, c, zeroram);
    aldo_console_set_snapshot(self, snp);
    *cn = self;
    return true;
}

//  TODO: combine with powerup and return error code
aldo_console *aldo_console_new(enum aldo_console_type type, aldo_debugger *dbg,
                               FILE *tracelog)
{
    assert(dbg != nullptr);

    switch (type) {
    case ALDO_CONSOLE_ALDO8:
        return new_aldo8(dbg, tracelog);
    case ALDO_CONSOLE_NES:
        return new_nes(dbg, tracelog);
        break;
    default:
        assert(((void)"INVALID CONSOLE TYPE", false));
        break;
    }
    return nullptr;
}

void aldo_console_free(aldo_console *self)
{
    assert(self != nullptr);

    teardown(self);
    free(self);
}

void aldo_console_powerup(aldo_console *self, aldo_cart *c, bool zeroram)
{
    assert(self != nullptr);
    assert(self->cart == nullptr);

    if (c) {
        connect_cart(self, c);
    }
    self->mode = ALDO_EXC_RUN;

    switch (self->type) {
    case ALDO_CONSOLE_ALDO8:
        aldo_aldo8_powerup((aldo_aldo8 *)self, zeroram);
        break;
    case ALDO_CONSOLE_NES:
        aldo_nes_powerup((aldo_nes *)self, zeroram);
        break;
    default:
        assert(((void)"INVALID CONSOLE TYPE", false));
        break;
    }
}

void aldo_console_powerdown(aldo_console *self)
{
    assert(self != nullptr);

    aldo_console_halt(self, true);
    disconnect_cart(self);
}

int aldo_console_max_tcpu()
{
    return Aldo_MaxTCycle;
}

const char *aldo_console_name(aldo_console *self)
{
    assert(self != nullptr);

    return typename(self->type);
}

size_t aldo_console_ram_size(aldo_console *self)
{
    assert(self != nullptr);

    return self->params.ramsize;
}

void aldo_console_screen_size(aldo_console *self, int *width, int *height)
{
    assert(self != nullptr);
    assert(width != nullptr);
    assert(height != nullptr);

    *width = self->params.screenw;
    *height = self->params.screenh;
}

int aldo_console_cycle_factor(aldo_console *self)
{
    assert(self != nullptr);

    return self->params.cyclefactor;
}

int aldo_console_frame_factor(aldo_console *self)
{
    assert(self != nullptr);

    return self->params.framefactor;
}

bool aldo_console_bcd_support(aldo_console *self)
{
    assert(self != nullptr);

    return self->cpu.bcd;
}

bool aldo_console_tracefailed(aldo_console *self)
{
    assert(self != nullptr);

    return self->tracefailed;
}

enum aldo_execmode aldo_console_mode(aldo_console *self)
{
    assert(self != nullptr);

    return self->mode;
}

void aldo_console_set_mode(aldo_console *self, enum aldo_execmode mode)
{
    assert(self != nullptr);

    // force signed to check < 0 (underlying type may be uint)
    self->mode = (int)mode < 0 ? ALDO_EXC_COUNT - 1 : mode % ALDO_EXC_COUNT;
}

bool aldo_console_halted(aldo_console *self)
{
    assert(self != nullptr);

    return self->halted;
}

void aldo_console_halt(aldo_console *self, bool halt)
{
    assert(self != nullptr);

    self->halted = halt;
}

bool aldo_console_probe(aldo_console *self, enum aldo_interrupt signal)
{
    assert(self != nullptr);

    switch (signal) {
    case ALDO_INT_IRQ:
        return self->probe.irq;
    case ALDO_INT_NMI:
        return self->probe.nmi;
    case ALDO_INT_RDY:
        return self->probe.rdy;
    case ALDO_INT_RST:
        return self->probe.rst;
    default:
        assert(((void)"INVALID NES PROBE", false));
        return false;
    }
}

void aldo_console_set_probe(aldo_console *self, enum aldo_interrupt signal,
                            bool active)
{
    assert(self != nullptr);

    switch (signal) {
    case ALDO_INT_IRQ:
        self->probe.irq = active;
        break;
    case ALDO_INT_NMI:
        self->probe.nmi = active;
        break;
    case ALDO_INT_RDY:
        self->probe.rdy = active;
        break;
    case ALDO_INT_RST:
        self->probe.rst = active;
        break;
    default:
        assert(((void)"INVALID NES PROBE", false));
        break;
    }
}

void aldo_console_clock(aldo_console *self, struct aldo_clock *clock)
{
    assert(self != nullptr);
    assert(clock != nullptr);

    if (aldo_console_halted(self)) return;

    reset_snapshot(self);
    while (clock->budget > 0 && !aldo_console_halted(self)) {
        clock_system(self, clock);
    }
    snapshot_core(self);
}

void aldo_console_set_snapshot(aldo_console *self, struct aldo_snapshot *snp)
{
    assert(self != nullptr);

    self->snp = snp;
    init_snapshot(self);
}

void aldo_console_dumpram(aldo_console *self, FILE *fs[static 3],
                          bool errs[static 3])
{
    assert(self != nullptr);
    assert(fs != nullptr);
    assert(errs != nullptr);

    // TODO: dump ram for all types
    assert(self->type == ALDO_CONSOLE_NES);
    aldo_nes_dumpram((aldo_nes *)self, fs, errs);
}

//
//  ppu.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 5/4/24.
//

#include "ciny.h"
#include "ctrlsignal.h"
#include "ppu.h"
#include "ppuhelp.h"

static void powerup_initializes_ppu(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);

    ppu_powerup(ppu);

    ct_assertequal(CSGS_PENDING, (int)ppu->rst);
    ct_assertequal(0u, ppu->regsel);
    ct_assertequal(0u, ppu->oamaddr);
    ct_assertfalse(ppu->signal.ale);
    ct_asserttrue(ppu->signal.intr);
    ct_asserttrue(ppu->signal.rst);
    ct_asserttrue(ppu->signal.rw);
    ct_asserttrue(ppu->signal.rd);
    ct_asserttrue(ppu->signal.wr);
    ct_assertfalse(ppu->signal.vout);
    ct_assertfalse(ppu->status.s);
}

//
// Reset
//

static void reset_sequence(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 42;
    ppu->dot = 24;
    ppu->signal.intr = false;
    ppu->ctrl.b = ppu->mask.b = ppu->signal.vout = ppu->cvp = ppu->odd =
        ppu->w = true;
    ppu->rbuf = 0x56;
    ppu->x = 0x78;
    ppu->t = ppu->v = 0x1234;

    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(25u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_CLEAR, (int)ppu->rst);

    ppu->signal.rst = false;
    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(26u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_DETECTED, (int)ppu->rst);

    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(27u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_PENDING, (int)ppu->rst);

    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(28u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_COMMITTED, (int)ppu->rst);

    // NOTE: reset line held
    for (int i = 0; i < 5; ++i) {
        ppu_cycle(ppu);
    }

    ct_assertequal(42u, ppu->line);
    ct_assertequal(33u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_COMMITTED, (int)ppu->rst);

    ppu->signal.rst = true;
    ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(1u, ppu->dot);
    ct_asserttrue(ppu->signal.intr);
    ct_assertfalse(ppu->ctrl.b);
    ct_assertfalse(ppu->mask.b);
    ct_assertfalse(ppu->signal.vout);
    ct_assertfalse(ppu->cvp);
    ct_assertfalse(ppu->odd);
    ct_assertfalse(ppu->w);
    ct_assertequal(0x0u, ppu->rbuf);
    ct_assertequal(0x0u, ppu->x);
    ct_assertequal(0x0u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_SERVICED, (int)ppu->rst);
}

static void reset_too_short(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 42;
    ppu->dot = 24;
    ppu->signal.intr = false;
    ppu->ctrl.b = ppu->mask.b = ppu->signal.vout = ppu->cvp = ppu->odd =
        ppu->w = true;
    ppu->rbuf = 0x56;
    ppu->x = 0x78;
    ppu->t = ppu->v = 0x1234;

    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(25u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_CLEAR, (int)ppu->rst);

    ppu->signal.rst = false;
    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(26u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_DETECTED, (int)ppu->rst);

    ppu->signal.rst = true;
    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->line);
    ct_assertequal(27u, ppu->dot);
    ct_assertfalse(ppu->signal.intr);
    ct_asserttrue(ppu->ctrl.b);
    ct_asserttrue(ppu->mask.b);
    ct_asserttrue(ppu->signal.vout);
    ct_asserttrue(ppu->cvp);
    ct_asserttrue(ppu->odd);
    ct_asserttrue(ppu->w);
    ct_assertequal(0x56u, ppu->rbuf);
    ct_assertequal(0x78u, ppu->x);
    ct_assertequal(0x1234u, ppu->t);
    ct_assertequal(0x1234u, ppu->v);
    ct_assertequal(CSGS_CLEAR, (int)ppu->rst);
}

//
// VBlank
//

static void vblank_prep(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = false;
    ppu->ctrl.v = true;
    ppu->line = 241;

    ppu_cycle(ppu);

    ct_assertequal(1u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_start(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = true;
    ppu->ctrl.v = true;
    ppu->line = 241;
    ppu->dot = 1;

    ppu_cycle(ppu);

    ct_assertequal(2u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);
}

static void vblank_start_nmi_disabled(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = true;
    ppu->line = 241;
    ppu->dot = 1;

    ppu_cycle(ppu);

    ct_assertequal(2u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_start_nmi_missed(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = false;
    ppu->ctrl.v = true;
    ppu->line = 241;
    ppu->dot = 1;

    ppu_cycle(ppu);

    ct_assertequal(2u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_nmi_toggle(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = true;
    ppu->ctrl.v = true;
    ppu->line = 250;
    ppu->dot = 40;
    ppu->signal.intr = false;

    ppu_cycle(ppu);

    ct_assertequal(41u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);

    ppu->ctrl.v = false;
    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);

    ppu->ctrl.v = true;
    ppu_cycle(ppu);

    ct_assertequal(43u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);
}

static void vblank_nmi_clear(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = true;
    ppu->ctrl.v = true;
    ppu->line = 250;
    ppu->dot = 40;
    ppu->signal.intr = false;

    ppu_cycle(ppu);

    ct_assertequal(41u, ppu->dot);
    ct_asserttrue(ppu->status.v);
    ct_assertfalse(ppu->signal.intr);

    ppu->status.v = false;
    ppu_cycle(ppu);

    ct_assertequal(42u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);

    ppu_cycle(ppu);

    ct_assertequal(43u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_asserttrue(ppu->signal.intr);
}

static void vblank_end(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->status.v = true;
    ppu->status.s = true;
    ppu->status.o = true;
    ppu->ctrl.v = true;
    ppu->line = 261;
    ppu->dot = 1;
    ppu->rst = CSGS_SERVICED;
    ppu->signal.intr = false;

    ppu_cycle(ppu);

    ct_assertequal(2u, ppu->dot);
    ct_assertfalse(ppu->status.v);
    ct_assertfalse(ppu->status.s);
    ct_assertfalse(ppu->status.o);
    ct_asserttrue(ppu->signal.intr);
    ct_assertequal(CSGS_CLEAR, (int)ppu->rst);
}

static void frame_toggle(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 261;
    ppu->dot = 340;

    ct_assertfalse(ppu->odd);

    ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(0u, ppu->dot);
    ct_asserttrue(ppu->odd);

    ppu->line = 261;
    ppu->dot = 340;
    ppu_cycle(ppu);

    ct_assertequal(0u, ppu->line);
    ct_assertequal(0u, ppu->dot);
    ct_assertfalse(ppu->odd);
}

//
// Trace
//

static void trace_no_adjustment(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);

    const struct ppu_coord pixel = ppu_trace(ppu, 0);

    ct_assertequal(0, pixel.dot);
    ct_assertequal(0, pixel.line);
}

static void trace_zero_with_adjustment(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);

    const struct ppu_coord pixel = ppu_trace(ppu, -3);

    ct_assertequal(338, pixel.dot);
    ct_assertequal(261, pixel.line);
}

static void trace(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 120;
    ppu->dot = 223;

    const struct ppu_coord pixel = ppu_trace(ppu, -3);

    ct_assertequal(220, pixel.dot);
    ct_assertequal(120, pixel.line);
}

static void trace_at_one_cpu_cycle(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->dot = 3;

    const struct ppu_coord pixel = ppu_trace(ppu, -3);

    ct_assertequal(0, pixel.dot);
    ct_assertequal(0, pixel.line);
}

static void trace_at_one_ppu_cycle(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 120;
    ppu->dot = 223;

    const struct ppu_coord pixel = ppu_trace(ppu, -1);

    ct_assertequal(222, pixel.dot);
    ct_assertequal(120, pixel.line);
}

static void trace_at_line_boundary(void *ctx)
{
    struct rp2c02 *const ppu = ppt_get_ppu(ctx);
    ppu->line = 120;
    ppu->dot = 1;

    const struct ppu_coord pixel = ppu_trace(ppu, -3);

    ct_assertequal(339, pixel.dot);
    ct_assertequal(119, pixel.line);
}

//
// Test List
//

struct ct_testsuite ppu_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(powerup_initializes_ppu),

        ct_maketest(reset_sequence),
        ct_maketest(reset_too_short),

        ct_maketest(vblank_prep),
        ct_maketest(vblank_start),
        ct_maketest(vblank_start_nmi_disabled),
        ct_maketest(vblank_start_nmi_missed),
        ct_maketest(vblank_nmi_toggle),
        ct_maketest(vblank_nmi_clear),
        ct_maketest(vblank_end),
        ct_maketest(frame_toggle),

        ct_maketest(trace_no_adjustment),
        ct_maketest(trace_zero_with_adjustment),
        ct_maketest(trace),
        ct_maketest(trace_at_one_cpu_cycle),
        ct_maketest(trace_at_one_ppu_cycle),
        ct_maketest(trace_at_line_boundary),
    };

    return ct_makesuite_setup_teardown(tests, ppu_setup, ppu_teardown);
}

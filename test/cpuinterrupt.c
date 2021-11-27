//
//  cpuinterrupt.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 4/14/21.
//

#include "bytes.h"
#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"

#include <stdint.h>
#include <stdlib.h>

//
// Interrupt Handling
//

static void interrupt_handler_setup(void **ctx)
{
    // NOTE: 32k rom, starting at $8000, for interrupt vectors
    uint8_t *const rom = calloc(MEMBLOCK_32KB, sizeof(uint8_t));
    rom[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xaa;
    rom[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0xbb;
    rom[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0x77;
    rom[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x99;
    rom[CPU_VECTOR_RES & ADDRMASK_32KB] = 0x22;
    rom[(CPU_VECTOR_RES + 1) & ADDRMASK_32KB] = 0x88;
    *ctx = rom;
}

static void interrupt_handler_teardown(void **ctx)
{
    free(*ctx);
}

static void brk_handler(void *ctx)
{
    uint8_t mem[] = {0x0, 0xff, 0xff, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0xbbaau, cpu.pc);

    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x34u, mem[509]);
    ct_assertequal(2u, mem[510]);
    ct_assertequal(0u, mem[511]);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void irq_handler(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;
    cpu.signal.irq = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0xbbaau, cpu.pc);

    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(3u, mem[510]);
    ct_assertequal(0u, mem[511]);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void nmi_handler(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.signal.nmi = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu.signal.nmi = true;
    cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0x9977u, cpu.pc);

    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x24u, mem[509]);
    ct_assertequal(3u, mem[510]);
    ct_assertequal(0u, mem[511]);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void res_handler(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {
        0xad, 0x4, 0x0, 0xff, 0x20, [509] = 0xdd, [510] = 0xee, [511] = 0xff,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.signal.res = false;

    cpu_cycle(&cpu);

    ct_assertequal(1u, cpu.pc);
    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(2u, cpu.pc);
    ct_assertequal(NIS_PENDING, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(2u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.res);

    cpu.signal.res = true;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0x8822u, cpu.pc);

    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0xddu, mem[509]);
    ct_assertequal(0xeeu, mem[510]);
    ct_assertequal(0xffu, mem[511]);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void rti_clear_irq_mask(void *ctx)
{
    uint8_t mem[] = {0x40, 0xff, 0xff, [258] = 0xb3, [259] = 0x5, [260] = 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 1;
    cpu.p.i = true;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(5u, cpu.pc);

    ct_assertequal(4u, cpu.s);
    ct_asserttrue(cpu.p.c);
    ct_asserttrue(cpu.p.z);
    ct_assertfalse(cpu.p.i);
    ct_assertfalse(cpu.p.d);
    ct_assertfalse(cpu.p.v);
    ct_asserttrue(cpu.p.n);
}

static void rti_set_irq_mask(void *ctx)
{
    uint8_t mem[] = {0x40, 0xff, 0xff, [258] = 0x7c, [259] = 0x5, [260] = 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 1;
    cpu.p.i = false;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(6, cycles);
    ct_assertequal(5u, cpu.pc);

    ct_assertequal(4u, cpu.s);
    ct_assertfalse(cpu.p.c);
    ct_assertfalse(cpu.p.z);
    ct_asserttrue(cpu.p.i);
    ct_asserttrue(cpu.p.d);
    ct_asserttrue(cpu.p.v);
    ct_assertfalse(cpu.p.n);
}

// NOTE: IRQ latched after soft BRK started;
// IRQ handler will see IRQ line active but B flag set
// and PC advanced past BRK instruction.
static void brk_masks_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0x0, 0xff, 0xff, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    for (int i = 0; i < 4; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_PENDING, (int)cpu.irq);
    }

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x30u, mem[509]);
    ct_assertequal(0x2u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    // NOTE: IRQ seen active again after first instruction
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: IRQ causes interrupt but line clears before sequence completes
// leading to an IRQ handler with nothing to service.
static void irq_ghost(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;
    cpu.signal.irq = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    cpu.signal.irq = true;
    cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0xbbaau, cpu.pc);

    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(3u, mem[510]);
    ct_assertequal(0u, mem[511]);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

// NOTE: NMI triggered and handler called but line never goes inactive
// to reset edge detection, so NMI latch never clears.
static void nmi_line_never_cleared(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    ((uint8_t *)ctx)[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.signal.nmi = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(250u, cpu.pc);

    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x24u, mem[509]);
    ct_assertequal(3u, mem[510]);
    ct_assertequal(0u, mem[511]);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI latched after soft BRK started;
// jumps to NMI handler but with B flag set
// and PC advanced beyond BRK instruction;
// BRK is lost if handler does not account for this case.
static void nmi_hijacks_brk(void *ctx)
{
    uint8_t mem[] = {0x0, 0xff, 0xff, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    for (int i = 0; i < 4; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(0x9977u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x34u, mem[509]);
    ct_assertequal(0x2u, mem[510]);
    ct_assertequal(0x0u, mem[511]);
}

// NOTE: NMI goes active on vector fetch cycle, delaying NMI detection until
// after first instruction of BRK handler.
static void nmi_delayed_by_brk(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0x0, 0xff, 0xff, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    for (int i = 0; i < 5; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x34u, mem[509]);
    ct_assertequal(0x2u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    // NOTE: NMI detected and handled after first instruction of BRK handler
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI goes active on final BRK cycle, delaying NMI detection until
// after first instruction of BRK handler.
static void nmi_late_delayed_by_brk(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0x0, 0xff, 0xff, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    for (int i = 0; i < 6; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x34u, mem[509]);
    ct_assertequal(0x2u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    // NOTE: NMI detected and handled after first instruction of BRK handler
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI goes active on vector fetch cycle, delaying NMI detection until
// after BRK sequence, but NMI goes inactive after first cycle of next
// instruction, losing the NMI.
static void nmi_lost_during_brk(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0x0, 0xff, 0xff, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    for (int i = 0; i < 5; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x34u, mem[509]);
    ct_assertequal(0x2u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu.signal.nmi = true;
    cpu_cycle(&cpu);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI latched after IRQ started, jumps to NMI handler instead
static void nmi_hijacks_irq(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    ((uint8_t *)ctx)[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 4; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI latched after IRQ started, jumps to NMI handler instead,
// IRQ signal goes inactive too early and IRQ is lost.
static void nmi_hijacks_and_loses_irq(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    ((uint8_t *)ctx)[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 4; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu.signal.irq = true;
    cpu_cycle(&cpu);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI goes active on vector fetch cycle, delaying NMI detection until
// after first instruction of IRQ handler.
static void nmi_delayed_by_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 5; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    // NOTE: NMI detected and handled after first instruction of BRK handler
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI goes active on final IRQ cycle, delaying NMI detection until
// after first instruction of IRQ handler.
static void nmi_late_delayed_by_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 6; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    // NOTE: NMI detected and handled after first instruction of BRK handler
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: NMI goes active on vector fetch cycle, delaying NMI detection until
// after IRQ sequence, but NMI goes inactive after first cycle of next
// instruction, losing the NMI.
static void nmi_lost_during_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 5; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    }

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    // NOTE: NMI detected and handled after first instruction of BRK handler
    cpu_cycle(&cpu);
    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);

    cpu.signal.nmi = true;
    cpu_cycle(&cpu);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: these tests do not reflect actual 6502 behavior, called
// "RESET half-hijacks", which exhibit all kinds of glitchy effects;
// however since no legit software would ever rely on this behavior and it is
// essentially impossible to recreate these conditions on functioning consumer
// hardware, it's easier to implement simpler logic that behaves consistently.

// NOTE: set RES on T4 prevents IRQ from finishing
static void res_hijacks_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 4; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.res);
    }

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(3u, cpu.pc);
    ct_assertfalse(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);
    ct_asserttrue(cpu.presync);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(3u, cpu.pc);
    ct_asserttrue(cpu.presync);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(3u, cpu.pc);
    ct_assertfalse(cpu.presync);
    ct_assertequal(0u, cpu.opc);
}

// NOTE: set RES on T5 triggers RES sequence immediately after IRQ sequence
static void res_following_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {
        0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, 0xea, 0xea, [511] = 0xff,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 5; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.res);
    }

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);
}

// NOTE: set RES on T6 still triggers reset immediately after IRQ sequence
static void res_late_on_irq(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_IRQ & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_IRQ + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {
        0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, 0xea, 0xea, [511] = 0xff,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;
    cpu.p.i = false;

    cpu.signal.irq = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    for (int i = 0; i < 6; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.res);
    }

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x20u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
    ct_assertequal(251u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(251u, cpu.pc);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(251u, cpu.pc);
}

// NOTE: set RES on T4 prevents NMI from finishing
static void res_hijacks_nmi(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, [511] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    cpu.signal.nmi = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    for (int i = 0; i < 4; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.res);
    }

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(3u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x24u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);
    ct_asserttrue(cpu.presync);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(3u, cpu.pc);
    ct_asserttrue(cpu.presync);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(3u, cpu.pc);
    ct_assertfalse(cpu.presync);
    ct_assertequal(0u, cpu.opc);
}

// NOTE: set RES on T5 triggers RES sequence immediately after NMI sequence
static void res_following_nmi(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {
        0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, 0xea, 0xea, [511] = 0xff,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    cpu.signal.nmi = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    for (int i = 0; i < 5; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.res);
    }

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x24u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);
}

// NOTE: set RES on T6 still triggers reset immediately after NMI sequence
static void res_late_on_nmi(void *ctx)
{
    ((uint8_t *)ctx)[CPU_VECTOR_NMI & ADDRMASK_32KB] = 0xfa;
    ((uint8_t *)ctx)[(CPU_VECTOR_NMI + 1) & ADDRMASK_32KB] = 0x0;
    uint8_t mem[] = {
        0xad, 0x4, 0x0, 0xea, 0x20, [250] = 0xea, 0xea, 0xea, [511] = 0xff,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, ctx);
    cpu.s = 0xff;

    cpu.signal.nmi = false;
    const int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    for (int i = 0; i < 6; ++i) {
        cpu_cycle(&cpu);
        ct_assertequal(NIS_CLEAR, (int)cpu.res);
    }

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);
    ct_assertequal(250u, cpu.pc);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(0xfcu, cpu.s);
    ct_assertequal(0x24u, mem[509]);
    ct_assertequal(0x3u, mem[510]);
    ct_assertequal(0x0u, mem[511]);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
    ct_assertequal(251u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(251u, cpu.pc);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(251u, cpu.pc);
}

//
// Interrupt Signal Detection
//

static void clear_on_startup(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void irq_poll_sequence(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
}

static void irq_short_sequence(void *ctx)
{
    // NOTE: LDA #$20
    uint8_t mem[] = {0xa9, 0x20, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
}

static void irq_long_sequence(void *ctx)
{
    // NOTE: DEC $0004 (0x20)
    uint8_t mem[] = {0xce, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
}

static void irq_branch_not_taken(void *ctx)
{
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = false;
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
}

static void irq_on_branch_taken_if_early_signal(void *ctx)
{
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(5u, cpu.pc);
}

static void irq_delayed_on_branch_taken_if_late_signal(void *ctx)
{
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu.p.i = false;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(5u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(6u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(6u, cpu.pc);
}

static void irq_delayed_on_infinite_branch(void *ctx)
{
    // NOTE: BEQ -2 jump to itself
    uint8_t mem[] = {0xf0, 0xfe, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu.p.i = false;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(0u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(0u, cpu.pc);
}

static void irq_on_branch_page_boundary(void *ctx)
{
    // NOTE: BEQ +5 -> $0101
    uint8_t mem[] = {[250] = 0xf0, 0x5, 0xff, [257] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu.p.i = false;
    cpu.pc = 250;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(251u, cpu.pc);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(252u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(257u, cpu.pc);
}

static void irq_just_in_time(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
}

static void irq_too_late(void *ctx)
{
    // NOTE: LDA $0006 (0x99), LDA #$20
    uint8_t mem[] = {0xad, 0x6, 0x0, 0xa9, 0x20, 0xff, 0x99};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(4u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(5u, cpu.pc);
}

static void irq_too_short(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.irq = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
}

static void irq_level_dependent(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu.signal.irq = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
}

static void irq_masked(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
}

static void irq_missed_by_sei(void *ctx)
{
    // NOTE: SEI
    uint8_t mem[] = {0x78, 0xff, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);
}

static void irq_missed_by_plp_set_mask(void *ctx)
{
    // NOTE: PLP (@ $0101)
    uint8_t mem[] = {0x28, 0xff, 0xff, 0xff, 0xff, [257] = 0x4};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);
}

static void irq_stopped_by_rti_set_mask(void *ctx)
{
    // NOTE: RTI (P @ $0101)
    uint8_t mem[] = {
        0x40, 0xff, 0xff, 0xff, 0xff, [257] = 0x4, [258] = 0x5, [259] = 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = false;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(5u, cpu.pc);
    ct_asserttrue(cpu.p.i);
}

static void irq_missed_by_cli(void *ctx)
{
    // NOTE: CLI, LDA #$20
    uint8_t mem[] = {0x58, 0xa9, 0x20, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = true;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
    ct_assertfalse(cpu.p.i);
}

static void irq_missed_by_plp_clear_mask(void *ctx)
{
    // NOTE: PLP (@ $0101), LDA #$20
    uint8_t mem[] = {0x28, 0xa9, 0x20, 0xff, 0xff, [257] = 0x0};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = true;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(3u, cpu.pc);
    ct_assertfalse(cpu.p.i);
}

static void irq_allowed_by_rti_clear_mask(void *ctx)
{
    // NOTE: RTI (P @ $0101)
    uint8_t mem[] = {
        0x40, 0xff, 0xff, 0xff, 0xff, [257] = 0x0, [258] = 0x5, [259] = 0x0,
    };
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.i = true;

    cpu.signal.irq = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.irq);
    ct_assertequal(1u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_asserttrue(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
    ct_assertequal(2u, cpu.pc);
    ct_assertfalse(cpu.p.i);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);
    ct_assertequal(5u, cpu.pc);
    ct_assertfalse(cpu.p.i);
}

static void irq_detect_duplicate(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    // NOTE: simulate irq held active during and after servicing interrupt
    cpu.signal.irq = false;
    cpu.irq = NIS_COMMITTED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    // NOTE: reset cpu to simulate interrupt has been serviced
    cpu.irq = NIS_CLEAR;
    cpu.pc = 0;
    cpu.presync = true;
    cpu_cycle(&cpu);

    // NOTE: if irq is still held active after servicing it'll
    // be detected again.
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
}

static void nmi_poll_sequence(void *ctx)
{
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);
}

static void nmi_short_sequence(void *ctx)
{
    // NOTE: LDA #$20
    uint8_t mem[] = {0xa9, 0x20, 0xff, 0xff, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);
}

static void nmi_long_sequence(void *ctx)
{
    // NOTE: DEC $0004 (0x20)
    uint8_t mem[] = {0xce, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);
}

static void nmi_branch_not_taken(void *ctx)
{
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = false;

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);
}

static void nmi_on_branch_taken_if_early_signal(void *ctx)
{
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(5u, cpu.pc);
}

static void nmi_delayed_on_branch_taken_if_late_signal(void *ctx)
{
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(5u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(6u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(6u, cpu.pc);
}

static void nmi_delayed_on_infinite_branch(void *ctx)
{
    // NOTE: BEQ -2 jump to itself
    uint8_t mem[] = {0xf0, 0xfe, 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(0u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(0u, cpu.pc);
}

static void nmi_on_branch_page_boundary(void *ctx)
{
    // NOTE: BEQ +5 -> $0101
    uint8_t mem[] = {[250] = 0xf0, 0x5, 0xff, [257] = 0xff};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu.pc = 250;

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(251u, cpu.pc);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(252u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(257u, cpu.pc);
}

static void nmi_just_in_time(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);
}

static void nmi_too_late(void *ctx)
{
    // NOTE: LDA $0006 (0x99), LDA #$20
    uint8_t mem[] = {0xad, 0x6, 0x0, 0xa9, 0x20, 0xff, 0x99};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(4u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);
    ct_assertequal(5u, cpu.pc);
}

static void nmi_too_short(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.nmi = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);
}

static void nmi_edge_persist(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.nmi = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.nmi);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(2u, cpu.pc);

    cpu.signal.nmi = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.nmi);
    ct_assertequal(3u, cpu.pc);
}

static void nmi_serviced_only_clears_on_inactive(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    // NOTE: simulate nmi held active during and after servicing interrupt
    cpu.signal.nmi = false;
    cpu.nmi = NIS_COMMITTED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    // NOTE: reset cpu to simulate interrupt has been serviced
    cpu.nmi = NIS_SERVICED;
    cpu.pc = 0;
    cpu.presync = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);

    // NOTE: nmi cannot be detected again until line goes inactive
    cpu.signal.nmi = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
}

static void res_detected_and_cpu_held(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);
    ct_assertequal(1u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);
}

static void res_too_short(void *ctx)
{
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    struct mos6502 cpu;
    setup_cpu(&cpu, mem, NULL);

    cpu.signal.res = false;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_DETECTED, (int)cpu.res);
    ct_assertequal(1u, cpu.pc);

    cpu.signal.res = true;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_CLEAR, (int)cpu.res);
    ct_assertequal(2u, cpu.pc);
}

//
// Test Lists
//

struct ct_testsuite cpu_interrupt_handler_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(brk_handler),
        ct_maketest(irq_handler),
        ct_maketest(nmi_handler),
        ct_maketest(res_handler),
        ct_maketest(rti_clear_irq_mask),
        ct_maketest(rti_set_irq_mask),

        ct_maketest(brk_masks_irq),
        ct_maketest(irq_ghost),

        ct_maketest(nmi_line_never_cleared),
        ct_maketest(nmi_hijacks_brk),
        ct_maketest(nmi_delayed_by_brk),
        ct_maketest(nmi_late_delayed_by_brk),
        ct_maketest(nmi_lost_during_brk),
        ct_maketest(nmi_hijacks_irq),
        ct_maketest(nmi_hijacks_and_loses_irq),
        ct_maketest(nmi_delayed_by_irq),
        ct_maketest(nmi_late_delayed_by_irq),
        ct_maketest(nmi_lost_during_irq),

        ct_maketest(res_hijacks_irq),
        ct_maketest(res_following_irq),
        ct_maketest(res_late_on_irq),
        ct_maketest(res_hijacks_nmi),
        ct_maketest(res_following_nmi),
        ct_maketest(res_late_on_nmi),
    };

    return ct_makesuite_setup_teardown(tests, interrupt_handler_setup,
                                       interrupt_handler_teardown);
}

struct ct_testsuite cpu_interrupt_signal_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(clear_on_startup),

        ct_maketest(irq_poll_sequence),
        ct_maketest(irq_short_sequence),
        ct_maketest(irq_long_sequence),
        ct_maketest(irq_branch_not_taken),
        ct_maketest(irq_on_branch_taken_if_early_signal),
        ct_maketest(irq_delayed_on_branch_taken_if_late_signal),
        ct_maketest(irq_delayed_on_infinite_branch),
        ct_maketest(irq_on_branch_page_boundary),
        ct_maketest(irq_just_in_time),
        ct_maketest(irq_too_late),
        ct_maketest(irq_too_short),
        ct_maketest(irq_level_dependent),
        ct_maketest(irq_masked),
        ct_maketest(irq_missed_by_sei),
        ct_maketest(irq_missed_by_plp_set_mask),
        ct_maketest(irq_stopped_by_rti_set_mask),
        ct_maketest(irq_missed_by_cli),
        ct_maketest(irq_missed_by_plp_clear_mask),
        ct_maketest(irq_allowed_by_rti_clear_mask),
        ct_maketest(irq_detect_duplicate),

        ct_maketest(nmi_poll_sequence),
        ct_maketest(nmi_short_sequence),
        ct_maketest(nmi_long_sequence),
        ct_maketest(nmi_branch_not_taken),
        ct_maketest(nmi_on_branch_taken_if_early_signal),
        ct_maketest(nmi_delayed_on_branch_taken_if_late_signal),
        ct_maketest(nmi_delayed_on_infinite_branch),
        ct_maketest(nmi_on_branch_page_boundary),
        ct_maketest(nmi_just_in_time),
        ct_maketest(nmi_too_late),
        ct_maketest(nmi_too_short),
        ct_maketest(nmi_edge_persist),
        ct_maketest(nmi_serviced_only_clears_on_inactive),

        ct_maketest(res_detected_and_cpu_held),
        ct_maketest(res_too_short),
    };

    return ct_makesuite(tests);
}

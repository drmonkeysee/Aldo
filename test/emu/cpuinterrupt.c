//
//  cpuinterrupt.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 4/14/21.
//

#include "ciny.h"
#include "cpuhelp.h"
#include "emu/cpu.h"
#include "emu/snapshot.h"
#include "emu/traits.h"

#include <stdint.h>
#include <stdlib.h>

//
// Interrupt Handling
//

static void interrupt_handler_setup(void **ctx)
{
    // NOTE: 32k rom, starting at $8000, for interrupt vectors
    uint8_t *const rom = calloc(0x8000, sizeof(uint8_t));
    rom[IrqVector & CpuCartAddrMask] = 0xaa;
    rom[(IrqVector + 1) & CpuCartAddrMask] = 0xbb;
    rom[NmiVector & CpuCartAddrMask] = 0x88;
    rom[(NmiVector + 1) & CpuCartAddrMask] = 0x99;
    rom[ResetVector & CpuCartAddrMask] = 0x44;
    rom[(ResetVector + 1) & CpuCartAddrMask] = 0x55;
    *ctx = rom;
}

static void interrupt_handler_teardown(void **ctx)
{
    free(*ctx);
}

static void brk_handler(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0x0, 0xff, 0xff, [511] = 0xff};
    cpu.ram = mem;
    cpu.cart = ctx;
    cpu.s = 0xff;

    const int cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0xbbaau, cpu.pc);

    ct_assertequal(0u, mem[511]);
    ct_assertequal(2u, mem[510]);
    ct_assertequal(0x34u, mem[509]);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void irq_handler(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20, [511] = 0xff};
    cpu.ram = mem;
    cpu.cart = ctx;
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

    ct_assertequal(0u, mem[511]);
    ct_assertequal(3u, mem[510]);
    ct_assertequal(0x20u, mem[509]);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void nmi_handler(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20, [511] = 0xff};
    cpu.ram = mem;
    cpu.cart = ctx;
    cpu.s = 0xff;
    cpu.signal.nmi = false;

    int cycles = clock_cpu(&cpu);

    ct_assertequal(4, cycles);
    ct_assertequal(3u, cpu.pc);
    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu.signal.nmi = true;
    cycles = clock_cpu(&cpu);

    ct_assertequal(7, cycles);
    ct_assertequal(0x9988u, cpu.pc);

    ct_assertequal(0u, mem[511]);
    ct_assertequal(3u, mem[510]);
    ct_assertequal(0x24u, mem[509]);
    ct_asserttrue(cpu.p.i);
    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_SERVICED, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void res_handler(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: IRQ committed after push P, ending up with BRK flag (lost IRQ)
static void brk_masks_irq(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: IRQ committed before push P, ending up with IRQ flag (lost BRK)
static void irq_hijacks_brk(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: NMI committed after push P of BRK (odd flag state for NMI, BRK lost if NMI doesn't handle this case)
static void nmi_with_brk_flag(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: NMI committed before push P of BRK (lost BRK)
static void nmi_hijacks_brk(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: NMI committed before vector select
static void nmi_hijacks_irq(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: NMI pending on vector select
static void nmi_delayed_by_irq(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: NMI pending on vector select but goes inactive before interrupt clear
static void nmi_lost_during_irq(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: set RES on on T4 to show hijack cannot be stopped
static void res_hijacks_irq(void *ctx)
{
    ct_assertfail("Not implemented");
}

// NOTE: set RES on on T4 to show hijack cannot be stopped
static void res_hijacks_nmi(void *ctx)
{
    ct_assertfail("Not implemented");
}

//
// Interrupt Signal Detection
//

static void clear_on_startup(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

    ct_assertequal(NIS_CLEAR, (int)cpu.irq);
    ct_assertequal(NIS_CLEAR, (int)cpu.nmi);
    ct_assertequal(NIS_CLEAR, (int)cpu.res);
}

static void irq_poll_sequence(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA #$20
    uint8_t mem[] = {0xa9, 0x20, 0xff, 0xff, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: DEC $0004 (0x20)
    uint8_t mem[] = {0xce, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ -2 jump to itself
    uint8_t mem[] = {0xf0, 0xfe, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +5 -> $0101
    uint8_t mem[] = {[250] = 0xf0, 0x5, 0xff, [257] = 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA $0006 (0x99), LDA #$20
    uint8_t mem[] = {0xad, 0x6, 0x0, 0xa9, 0x20, 0xff, 0x99};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: SEI
    uint8_t mem[] = {0x78, 0xff, 0xff, 0xff, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: PLP (@ $0101)
    uint8_t mem[] = {0x28, 0xff, 0xff, 0xff, 0xff, [257] = 0x4};
    cpu.ram = mem;
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

static void irq_missed_by_cli(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: CLI, LDA #$20
    uint8_t mem[] = {0x58, 0xa9, 0x20, 0xff, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: PLP (@ $0101), LDA #$20
    uint8_t mem[] = {0x28, 0xa9, 0x20, 0xff, 0xff, [257] = 0x0};
    cpu.ram = mem;
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

static void irq_detect_duplicate(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

    // NOTE: simulate irq held active during and after servicing interrupt
    cpu.signal.irq = false;
    cpu.irq = NIS_COMMITTED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.irq);

    cpu.irq = NIS_CLEAR;
    cpu_cycle(&cpu);

    // NOTE: if irq is still held active after servicing it'll
    // be detected again.
    ct_assertequal(NIS_DETECTED, (int)cpu.irq);

    cpu_cycle(&cpu);

    ct_assertequal(NIS_PENDING, (int)cpu.irq);
}

static void nmi_poll_sequence(void *ctx)
{
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA $0004 (0x20)
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA #$20
    uint8_t mem[] = {0xa9, 0x20, 0xff, 0xff, 0xff};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: DEC $0004 (0x20)
    uint8_t mem[] = {0xce, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +3 jump to SEC
    uint8_t mem[] = {0xf0, 0x3, 0xff, 0xff, 0xff, 0x38, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ -2 jump to itself
    uint8_t mem[] = {0xf0, 0xfe, 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: BEQ +5 -> $0101
    uint8_t mem[] = {[250] = 0xf0, 0x5, 0xff, [257] = 0xff};
    cpu.ram = mem;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    // NOTE: LDA $0006 (0x99), LDA #$20
    uint8_t mem[] = {0xad, 0x6, 0x0, 0xa9, 0x20, 0xff, 0x99};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

    // NOTE: simulate nmi held active during and after servicing interrupt
    cpu.signal.nmi = false;
    cpu.nmi = NIS_COMMITTED;
    cpu_cycle(&cpu);

    ct_assertequal(NIS_COMMITTED, (int)cpu.nmi);

    cpu.nmi = NIS_SERVICED;
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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
    struct mos6502 cpu;
    setup_cpu(&cpu);
    uint8_t mem[] = {0xad, 0x4, 0x0, 0xff, 0x20};
    cpu.ram = mem;

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
        ct_maketest(brk_masks_irq),
        ct_maketest(irq_hijacks_brk),
        ct_maketest(nmi_with_brk_flag),
        ct_maketest(nmi_hijacks_brk),
        ct_maketest(nmi_hijacks_irq),
        ct_maketest(nmi_delayed_by_irq),
        ct_maketest(nmi_lost_during_irq),
        ct_maketest(res_hijacks_irq),
        ct_maketest(res_hijacks_nmi),
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
        ct_maketest(irq_missed_by_cli),
        ct_maketest(irq_missed_by_plp_clear_mask),
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

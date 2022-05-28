//
//  cpu.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#include "cpu.h"

#include "bytes.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

// NOTE: sentinel value for cycle count denoting an imminent opcode fetch
static const int PreFetch = -1;

//
// State Management
//

// NOTE: Pool is never freed, global pointer is reachable at program exit
// WARNING: moving contexts in/out of the Pool is not thread-safe!
static struct cpu_context {
    struct cpu_context *next;
    struct mos6502 cpu;
} *Pool;

static struct cpu_context *capture(struct mos6502 *self)
{
    struct cpu_context *ctx;
    if (Pool) {
        ctx = Pool;
        Pool = Pool->next;
    } else {
        ctx = malloc(sizeof *ctx);
    }
    *ctx = (struct cpu_context){.cpu = *self};
    return ctx;
}

static void restore(struct mos6502 *self, struct cpu_context *ctx)
{
    *self = ctx->cpu;
    ctx->cpu.bus = NULL;    // Clean up any dangling pointers
    ctx->next = Pool;
    Pool = ctx;
}

static void detach(struct mos6502 *self)
{
    // TODO: adjust any bus devices with side-effect reads
    self->detached = true;
}

static void attach(struct mos6502 *self)
{
    // TODO: adjust any bus devices with side-effect reads
    self->detached = false;
}

//
// Internal Operations
//

static void read(struct mos6502 *self)
{
    assert(self->bus != NULL);

    self->signal.rw = true;
    self->bflt = !bus_read(self->bus, self->addrbus, &self->databus);
}

static void write(struct mos6502 *self)
{
    assert(self->bus != NULL);

    if (self->detached) {
        read(self);
    } else {
        self->signal.rw = false;
        self->bflt = !bus_write(self->bus, self->addrbus, self->databus);
    }
}

static uint8_t get_p(const struct mos6502 *self, bool interrupt)
{
    uint8_t p = 0x20;       // Unused bit is always set
    p |= self->p.c << 0;
    p |= self->p.z << 1;
    p |= self->p.i << 2;
    p |= self->p.d << 3;
    p |= !interrupt << 4;   // B bit is 0 if interrupt, 1 otherwise
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
    // NOTE: skip B and unused flags, they cannot be set explicitly
    self->p.v = p & 0x40;
    self->p.n = p & 0x80;
}

static void update_z(struct mos6502 *self, uint8_t d)
{
    self->p.z = d == 0;
}

static bool bcd_mode(struct mos6502 *self)
{
    return self->bcd && self->p.d;
}

// NOTE: signed overflow happens when positive + positive = negative
// or negative + negative = positive, i.e. the sign of A does not match
// the sign of S and the sign of B does not match the sign of S:
// (Sign A ^ Sign S) & (Sign B ^ Sign S) or
// (A ^ S) & (B ^ S) & SignMask
static void update_v(struct mos6502 *self, uint8_t s, uint8_t a, uint8_t b)
{
    self->p.v = (a ^ s) & (b ^ s) & 0x80;
}

static void update_n(struct mos6502 *self, uint8_t d)
{
    self->p.n = d & 0x80;
}

//
// Interrupt Detection
//

// NOTE: check interrupt lines (active low) on ϕ2 to
// initiate interrupt detection.
static void check_interrupts(struct mos6502 *self)
{
    // NOTE: serviced state is only for assisting in nmi edge detection
    assert(self->res != NIS_SERVICED);
    assert(self->irq != NIS_SERVICED);

    if (!self->signal.res && self->res == NIS_CLEAR) {
        self->res = NIS_DETECTED;
    }

    // NOTE: final cycle of BRK sequence holds interrupt latching low,
    // delaying detection of any still-active interrupt signals by one cycle
    // (except RES which is all-powerful).
    if (self->opc == BrkOpcode && self->t == 6) return;

    if (self->signal.nmi) {
        if (self->nmi == NIS_SERVICED) {
            self->nmi = NIS_CLEAR;
        }
    } else if (self->nmi == NIS_CLEAR) {
        self->nmi = NIS_DETECTED;
    }

    if (!self->signal.irq && self->irq == NIS_CLEAR) {
        self->irq = NIS_DETECTED;
    }
}

// NOTE: check persistence of interrupt lines (active low) on ϕ1
// of following cycle to latch interrupt detection and make polling possible.
static void latch_interrupts(struct mos6502 *self)
{
    // NOTE: res is level-detected but unlike irq it takes effect at
    // end of this cycle's ϕ2 so it moves directly from pending in this cycle
    // to committed in the next cycle.
    if (self->signal.res) {
        if (self->res == NIS_DETECTED) {
            self->res = NIS_CLEAR;
        }
    } else if (self->res == NIS_DETECTED) {
        self->res = NIS_PENDING;
    }

    // NOTE: nmi is edge-detected so once it has latched in it remains
    // set until serviced by a handler or reset.
    if (self->signal.nmi) {
        if (self->nmi == NIS_DETECTED) {
            self->nmi = NIS_CLEAR;
        }
    } else if (self->nmi == NIS_DETECTED) {
        self->nmi = NIS_PENDING;
    }

    // NOTE: irq is level-detected so must remain low until polled
    // or the interrupt is lost.
    if (self->signal.irq) {
        if (self->irq == NIS_DETECTED || self->irq == NIS_PENDING) {
            self->irq = NIS_CLEAR;
        }
    } else if (self->irq == NIS_DETECTED) {
        self->irq = NIS_PENDING;
    }
}

static void poll_interrupts(struct mos6502 *self)
{
    if (self->detached) return;

    if (self->nmi == NIS_PENDING) {
        self->nmi = NIS_COMMITTED;
    }

    if (self->irq == NIS_PENDING && !self->p.i) {
        self->irq = NIS_COMMITTED;
    }
}

static bool service_interrupt(struct mos6502 *self)
{
    return self->res == NIS_COMMITTED
            || self->nmi == NIS_COMMITTED
            || self->irq == NIS_COMMITTED;
}

static uint16_t interrupt_vector(struct mos6502 *self)
{
    if (self->res == NIS_COMMITTED) return CPU_VECTOR_RES;
    if (self->nmi == NIS_COMMITTED) return CPU_VECTOR_NMI;
    return CPU_VECTOR_IRQ;
}

// NOTE: res takes effect after two cycles and immediately resets/halts
// the cpu until the res line goes high again (this isn't strictly true
// but the real cpu complexity isn't necessary here).
static bool reset_held(struct mos6502 *self)
{
    if (self->res == NIS_PENDING) {
        self->res = NIS_COMMITTED;
        detach(self);
        self->presync = true;
    }
    return self->res == NIS_COMMITTED && !self->signal.res;
}

//
// Instruction Execution
//

// NOTE: finish an instruction by polling for interrupts and signaling the
// next cycle to be an opcode fetch; the order in which this is called
// simulates the side-effects of 6502's pipelining behavior without actually
// emulating the cycle timing, for example:
//
// SEI is setting the I flag from 0 to 1, disabling interrupts; SEI is a
// 2-cycle instruction and therefore polls for interrupts on the T1 (final)
// cycle, however the I flag is not actually set until T0 of the *next* cycle,
// so if interrupt-polling finds an active interrupt it will insert a BRK at
// the end of SEI and an interrupt will fire despite following the
// "disable interrupt" instruction; correspondingly CLI will delay an
// interrupt for an extra instruction for the same reason;

// committing the operation before executing the register side-effects will
// emulate this pipeline timing with respect to interrupts without modelling
// the actual cycle-delay of internal CPU operations.
static void conditional_commit(struct mos6502 *self, bool c)
{
    // NOTE: interrupts are polled regardless of commit condition
    poll_interrupts(self);
    self->presync = c;
}

static void commit_operation(struct mos6502 *self)
{
    conditional_commit(self, true);
}

static void load_register(struct mos6502 *self, uint8_t *r, uint8_t d)
{
    *r = d;
    update_z(self, *r);
    update_n(self, *r);
}

static void store_data(struct mos6502 *self, uint8_t d)
{
    self->databus = d;
    write(self);
}

// NOTE: add with carry-in: A + B + C and subtract with carry-in: A - B - ~C,
// where ~carry indicates borrow-out, are equivalent in binary mode:
//  A - B => A + (-B) => A + 2sComplement(B) => A + (~B + 1) when C = 1;
//  C = 0 is thus a borrow-out; A + (~B + 0) => A + ~B => A - B - 1.
static void binary_add(struct mos6502 *self, uint8_t a, uint8_t b, uint8_t c)
{
    const uint16_t sum = a + b + c;
    self->p.c = sum & 0x100;
    update_v(self, sum, a, b);
    load_register(self, &self->a, sum);
}

static void decimal_add(struct mos6502 *self, uint8_t alo, uint8_t blo,
                        uint8_t ahi, uint8_t bhi, bool c)
{
    uint8_t slo = alo + blo + c;
    const bool chi = slo > 0x9;
    if (chi) {
        slo += 0x6;
        slo &= 0xf;
    }
    uint8_t shi = ahi + bhi + chi;
    // NOTE: overflow and negative are set before decimal adjustment
    update_v(self, shi << 4, ahi << 4, bhi << 4);
    update_n(self, shi << 4);
    self->p.c = shi > 0x9;
    if (self->p.c) {
        shi += 0x6;
        shi &= 0xf;
    }
    self->a = (shi << 4) | slo;
}

static void decimal_subtract(struct mos6502 *self, uint8_t alo, uint8_t blo,
                             uint8_t ahi, uint8_t bhi, uint8_t brw)
{
    uint8_t dlo = alo - blo + brw;  // borrow is either 0 or -1
    const bool brwhi = dlo >= 0x80; // dlo < 0
    if (brwhi) {
        dlo -= 0x6;
        dlo &= 0xf;
    }
    uint8_t dhi = ahi - bhi - brwhi;
    if (dhi >= 0x80) {  // dhi < 0
        dhi -= 0x6;
        dhi &= 0xf;
    }
    // NOTE: bcd subtract does not adjust flags from earlier binary op
    self->a = (dhi << 4) | dlo;
}

enum arithmetic_operator {
    AOP_ADD,
    AOP_SUB,
};

static void arithmetic_operation(struct mos6502 *self,
                                 enum arithmetic_operator op, uint8_t b)
{
    const uint8_t a = self->a;
    const bool c = self->p.c;
    // NOTE: even in BCD mode some flags are set as if in binary mode
    // so always do binary op regardless of BCD flag.
    binary_add(self, a, op == AOP_SUB ? ~b : b, c);

    if (!bcd_mode(self)) return;

    // NOTE: decimal mode is 4-bit nibble arithmetic with carry/borrow
    const uint8_t
        alo = a & 0xf,
        blo = b & 0xf,
        ahi = (a >> 4) & 0xf,
        bhi = (b >> 4) & 0xf;
    if (op == AOP_SUB) {
        const uint8_t borrow = c - 1;
        decimal_subtract(self, alo, blo, ahi, bhi, borrow);
    } else {
        decimal_add(self, alo, blo, ahi, bhi, c);
    }
}

// NOTE: compare is effectively R - D; modeling the subtraction as
// R + 2sComplement(D) gets us all the flags for free;
// see binary_add for why this works.
static uint8_t compare_register(struct mos6502 *self, uint8_t r, uint8_t d)
{
    const uint16_t cmp = r + (uint8_t)~d + 1;
    self->p.c = cmp & 0x100;
    update_z(self, cmp);
    update_n(self, cmp);
    return cmp;
}

static void modify_mem(struct mos6502 *self, uint8_t d)
{
    store_data(self, d);
    update_z(self, d);
    update_n(self, d);
}

enum bitdirection {
    BIT_LEFT,
    BIT_RIGHT,
};

static uint8_t bitoperation(struct mos6502 *self, struct decoded dec,
                            enum bitdirection bd, uint8_t carryin_mask)
{
    // NOTE: some unofficial shift/rotate opcodes use immediate mode
    // to operate on the accumulator.
    const bool acc_operand = dec.mode == AM_IMP || dec.mode == AM_IMM;
    uint8_t d = acc_operand ? self->a : self->databus;
    if (bd == BIT_LEFT) {
        self->p.c = d & 0x80;
        d <<= 1;
    } else {
        self->p.c = d & 0x1;
        d >>= 1;
    }
    d |= carryin_mask;

    if (acc_operand) {
        load_register(self, &self->a, d);
    } else {
        modify_mem(self, d);
    }
    return d;
}

static void stack_top(struct mos6502 *self)
{
    self->addrbus = bytowr(self->s, 0x1);
    read(self);
}

static void stack_pop(struct mos6502 *self)
{
    ++self->s;
    stack_top(self);
}

static void stack_push(struct mos6502 *self, uint8_t d)
{
    self->addrbus = bytowr(self->s--, 0x1);
    store_data(self, d);
}

// NOTE: all 6502 cycles are either a read or a write, some of them discarded
// depending on the instruction and addressing-mode timing; these extra reads
// and writes are all modeled below to help verify cycle-accurate behavior.

static bool read_delayed(struct mos6502 *self, struct decoded dec,
                         bool delay_condition)
{
    if (!delay_condition) return false;

    bool delayed;
    switch (dec.mode) {
    case AM_INDY:
        delayed = self->t == 4;
        break;
    case AM_ABSX:
    case AM_ABSY:
        delayed = self->t == 3;
        break;
    default:
        delayed = false;
        break;
    }
    if (delayed) {
        read(self);
    }
    return delayed;
}

static bool write_delayed(struct mos6502 *self, struct decoded dec)
{
    bool delayed;
    switch (dec.mode) {
    case AM_ZP:
        delayed = self->t == 2;
        break;
    case AM_ZPX:
    case AM_ZPY:
    case AM_ABS:
        delayed = self->t == 3;
        break;
    case AM_ABSX:
    case AM_ABSY:
        delayed = self->t == 4;
        break;
    case AM_INDX:
    case AM_INDY:
        delayed = self->t == 5;
        break;
    default:
        delayed = false;
        break;
    }
    if (delayed) {
        read(self);
    }
    return delayed;
}

static void UDF_exec(struct mos6502 *self)
{
    commit_operation(self);
}

static void ADC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    arithmetic_operation(self, AOP_ADD, self->databus);
}

static void AND_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a & self->databus);
}

static void ASL_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_LEFT, 0x0);
}

// NOTE: branch instructions do not branch if the opposite condition is TRUE;
// e.g. BCC (branch on carry clear) will NOT branch if carry is SET;
// i.e. NOT(Condition) => commit instruction.

static void BCC_exec(struct mos6502 *self)
{
    conditional_commit(self, self->p.c);
}

static void BCS_exec(struct mos6502 *self)
{
    conditional_commit(self, !self->p.c);
}

static void BEQ_exec(struct mos6502 *self)
{
    conditional_commit(self, !self->p.z);
}

static void BIT_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    update_z(self, self->a & self->databus);
    self->p.v = self->databus & 0x40;
    update_n(self, self->databus);
}

static void BMI_exec(struct mos6502 *self)
{
    conditional_commit(self, !self->p.n);
}

static void BNE_exec(struct mos6502 *self)
{
    conditional_commit(self, self->p.z);
}

static void BPL_exec(struct mos6502 *self)
{
    conditional_commit(self, self->p.n);
}

static void BRK_exec(struct mos6502 *self)
{
    // NOTE: res is only cleared by its own handler;
    // nmi is serviced by its own handler (for edge detection)
    // but cleared by all others;
    // irq is cleared by all handlers.
    if (self->res == NIS_COMMITTED) {
        attach(self);
        self->res = NIS_CLEAR;
    }
    self->nmi = self->nmi == NIS_COMMITTED ? NIS_SERVICED : NIS_CLEAR;
    self->irq = NIS_CLEAR;
    self->p.i = true;
    self->pc = bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void BVC_exec(struct mos6502 *self)
{
    conditional_commit(self, self->p.v);
}

static void BVS_exec(struct mos6502 *self)
{
    conditional_commit(self, !self->p.v);
}

static void CLC_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.c = false;
}

static void CLD_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.d = false;
}

static void CLI_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.i = false;
}

static void CLV_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.v = false;
}

static void CMP_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    compare_register(self, self->a, self->databus);
}

static void CPX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    compare_register(self, self->x, self->databus);
}

static void CPY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    compare_register(self, self->y, self->databus);
}

static void DEC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    modify_mem(self, self->databus - 1);
}

static void DEX_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->x - 1);
}

static void DEY_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->y, self->y - 1);
}

static void EOR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a ^ self->databus);
}

static void INC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    modify_mem(self, self->databus + 1);
}

static void INX_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->x + 1);
}

static void INY_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->y, self->y + 1);
}

static void JMP_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void JSR_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void LDA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->databus);
}

static void LDX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->x, self->databus);
}

static void LDY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->y, self->databus);
}

static void LSR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_RIGHT, 0x0);
}

static void NOP_exec(struct mos6502 *self, struct decoded dec)
{
    // NOTE: unofficial NOPs have reads triggered by
    // non-implied addressing modes.
    if (read_delayed(self, dec, self->adc)) return;
    if (dec.mode != AM_IMP) {
        read(self);
    }
    commit_operation(self);
}

static void ORA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a | self->databus);
}

static void PHA_exec(struct mos6502 *self)
{
    stack_push(self, self->a);
    commit_operation(self);
}

static void PHP_exec(struct mos6502 *self)
{
    stack_push(self, get_p(self, false));
    commit_operation(self);
}

static void PLA_exec(struct mos6502 *self)
{
    stack_pop(self);
    commit_operation(self);
    load_register(self, &self->a, self->databus);
}

static void PLP_exec(struct mos6502 *self)
{
    stack_pop(self);
    commit_operation(self);
    set_p(self, self->databus);
}

static void ROL_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_LEFT, self->p.c);
}

static void ROR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_RIGHT, self->p.c << 7);
}

static void RTI_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void RTS_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->adl, self->databus);
}

static void SBC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    arithmetic_operation(self, AOP_SUB, self->databus);
}

static void SEC_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.c = true;
}

static void SED_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.d = true;
}

static void SEI_exec(struct mos6502 *self)
{
    commit_operation(self);
    self->p.i = true;
}

static void STA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->a);
    commit_operation(self);
}

static void STX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->x);
    commit_operation(self);
}

static void STY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->y);
    commit_operation(self);
}

static void TAX_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->a);
}

static void TAY_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->y, self->a);
}

static void TSX_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->s);
}

static void TXA_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->a, self->x);
}

static void TXS_exec(struct mos6502 *self)
{
    self->s = self->x;
    commit_operation(self);
}

static void TYA_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->a, self->y);
}

//
// Unofficial Instructions
//

// NOTE: magic constant that interferes with accumulator varies based on
// chip manufacture, temperature, state of chip control signals,
// and maybe other unknown factors;
// https://csdb.dk/release/?id=212346 recommends using EE.
static const uint8_t Magic = 0xee;

static void store_unstable_addresshigh(struct mos6502 *self, uint8_t d)
{
    // NOTE: if addr carry, +1 has already been stored into adh
    const uint8_t adrhi = self->adc ? self->adh : self->adh + 1;
    d &= adrhi;
    // NOTE: on page-cross boundary this *occasionally* throws the calculated
    // value into ADDR_HI; emulate that behavior consistently to emphasize the
    // instability of these instructions.
    if (self->adc) {
        self->adh = d;
        self->addrbus = bytowr(self->adl, self->adh);
    }
    store_data(self, d);
}

static void ALR_exec(struct mos6502 *self, struct decoded dec)
{
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a & self->databus);
    bitoperation(self, dec, BIT_RIGHT, 0x0);
}

static void ANC_exec(struct mos6502 *self)
{
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a & self->databus);
    self->p.c = self->a & 0x80;
}

static void ANE_exec(struct mos6502 *self)
{
    read(self);
    commit_operation(self);
    load_register(self, &self->a, (self->a | Magic) & self->x & self->databus);
}

static void ARR_exec(struct mos6502 *self, struct decoded dec)
{
    read(self);
    commit_operation(self);
    // NOTE: not sure this is entirely correct, community docs do not
    // completely agree on exact sequence of side-effects, but the main
    // operations are AND + ROR while the invocation of the adder
    // causes the overflow and carry flags to act strangely;
    // https://csdb.dk/release/?id=212346 has the clearest description:
    //  A := A AND operand
    //  set overflow to xor of bit 7 and bit 6 of A (side-effect of adder)
    //  set carry to bit 7 of A (side-effect of adder)
    //      but hold the value until after rotate
    //  rotate A right but leave carry unaffected
    //      implemented as a standard ROR and then
    //      setting carry to held value from ADD/ADC step
    const uint8_t and_result = self->a & self->databus;
    load_register(self, &self->a, and_result);
    self->p.v = (self->a >> 7) ^ ((self->a >> 6) & 0x1);
    const bool c = self->a & 0x80;
    bitoperation(self, dec, BIT_RIGHT, self->p.c << 7);
    self->p.c = c;

    if (!bcd_mode(self)) return;

    // NOTE: once again https://csdb.dk/release/?id=212346 has the best
    // description of how BCD affects the final result:
    //  if low nibble of AND result + lsb of low nibble > 0x5,
    //      adjust A by 0x6 but throw away carry
    //  if high nibble of AND result + lsb of high nibble > 0x50,
    //      adjust A by 0x60 and set carry flag
    if ((and_result & 0xf) + (and_result & 0x1) > 0x5) {
        self->a = (self->a & 0xf0) | ((self->a + 0x6) & 0xf);
    }
    if ((and_result & 0xf0) + (and_result & 0x10) > 0x50) {
        self->a = ((self->a + 0x60) & 0xf0) | (self->a & 0xf);
        self->p.c = true;
    }
}

static void DCP_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    const uint8_t d = self->databus - 1;
    modify_mem(self, d);
    compare_register(self, self->a, d);
}

static void ISC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    const uint8_t d = self->databus + 1;
    modify_mem(self, d);
    arithmetic_operation(self, AOP_SUB, d);
}

static void JAM_exec(struct mos6502 *self)
{
    self->databus = 0xff;
    self->addrbus = bytowr(0xff, 0xff);
}

static void LAS_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    self->s &= self->databus;
    load_register(self, &self->a, self->s);
    load_register(self, &self->x, self->s);
}

static void LAX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->databus);
    load_register(self, &self->x, self->databus);
}

static void LXA_exec(struct mos6502 *self)
{
    read(self);
    commit_operation(self);
    const uint8_t d = (self->a | Magic) & self->databus;
    load_register(self, &self->a, d);
    load_register(self, &self->x, d);
}

static void RLA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    const uint8_t d = bitoperation(self, dec, BIT_LEFT, self->p.c);
    load_register(self, &self->a, self->a & d);
}

static void RRA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    const uint8_t d = bitoperation(self, dec, BIT_RIGHT, self->p.c << 7);
    arithmetic_operation(self, AOP_ADD, d);
}

static void SAX_exec(struct mos6502 *self)
{
    store_data(self, self->a & self->x);
    commit_operation(self);
}

static void SBX_exec(struct mos6502 *self)
{
    read(self);
    commit_operation(self);
    const uint8_t cmp = compare_register(self, self->a & self->x,
                                         self->databus);
    load_register(self, &self->x, cmp);
}

static void SHA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_unstable_addresshigh(self, self->a & self->x);
    commit_operation(self);
}

static void SHX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_unstable_addresshigh(self, self->x);
    commit_operation(self);
}

static void SHY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_unstable_addresshigh(self, self->y);
    commit_operation(self);
}

static void SLO_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    const uint8_t d = bitoperation(self, dec, BIT_LEFT, 0x0);
    load_register(self, &self->a, self->a | d);
}

static void SRE_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    const uint8_t d = bitoperation(self, dec, BIT_RIGHT, 0x0);
    load_register(self, &self->a, self->a ^ d);
}

static void TAS_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    self->s = self->a & self->x;
    store_unstable_addresshigh(self, self->s);
    commit_operation(self);
}

//
// Instruction Dispatch
//

static void dispatch_instruction(struct mos6502 *self, struct decoded dec)
{
    switch (dec.instruction) {
#define X(s, ...) case IN_ENUM(s): s##_exec(__VA_ARGS__); break;
        DEC_INST_X
#undef X
    default:
        assert(((void)"BAD INSTRUCTION DISPATCH", false));
        break;
    }
}

//
// Addressing Mode Sequences
//

// NOTE: addressing sequences approximate the cycle-by-cycle behavior of the
// 6502 processor; each cycle generally breaks down as follows:
// 1. put current cycle's address on the addressbus (roughly ϕ1)
// 2. perform internal addressing operations for next cycle
//    (latching previous databus contents, adding address offsets, etc.)
// 3. execute read/write/instruction, either reading data to databus
//    or writing data from databus (roughly ϕ2)
// Note that the actual 6502 executes instruction side-effects on T0/T1 of the
// next instruction but we execute all side-effects within the current
// instruction sequence (generally the last cycle); in other words we don't
// emulate the 6502's simple pipelining.

#define BAD_ADDR_SEQ assert(((void)"BAD ADDRMODE SEQUENCE", false))

static void zeropage_indexed(struct mos6502 *self, struct decoded dec,
                             uint8_t index)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = bytowr(self->databus, 0x0);
        self->adl = self->databus + index;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl, 0x0);
        dispatch_instruction(self, dec);
        break;
    case 4:
        write(self);
        break;
    case 5:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void absolute_indexed(struct mos6502 *self, struct decoded dec,
                             uint8_t index)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = self->pc++;
        self->adl = self->databus + index;
        self->adc = self->adl < index;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl, self->databus);
        self->adh = self->databus + self->adc;
        dispatch_instruction(self, dec);
        break;
    case 4:
        self->addrbus = bytowr(self->adl, self->adh);
        dispatch_instruction(self, dec);
        break;
    case 5:
        write(self);
        break;
    case 6:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void branch_displacement(struct mos6502 *self)
{
    self->adl = self->pc + self->databus;
    // NOTE: branch uses signed displacement so there are three overflow cases:
    // no overflow = no adjustment to pc-high;
    // positive overflow = carry-in to pc-high => pch + 1;
    // negative overflow = borrow-out from pc-high => pch - 1;
    // subtracting -overflow condition from +overflow condition results in:
    // no overflow = 0 - 0 => pch + 0,
    // +overflow = 1 - 0 => pch + 1,
    // -overflow = 0 - 1 => pch - 1 => pch + 2sComplement(1) => pch + 0xff.
    const bool
        negative_offset = self->databus & 0x80,
        positive_overflow = self->adl < self->databus && !negative_offset,
        negative_overflow = self->adl > self->databus && negative_offset;
    self->adc = positive_overflow - negative_overflow;
    self->pc = bytowr(self->adl, self->pc >> 8);
}

static void branch_carry(struct mos6502 *self)
{
    self->pc = bytowr(self->adl, (self->pc >> 8) + self->adc);
}

static void IMP_sequence(struct mos6502 *self, struct decoded dec)
{
    assert(self->t == 1);

    self->addrbus = self->pc;
    read(self);
    dispatch_instruction(self, dec);
}

static void IMM_sequence(struct mos6502 *self, struct decoded dec)
{
    assert(self->t == 1);

    self->addrbus = self->pc++;
    dispatch_instruction(self, dec);
}

static void ZP_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = bytowr(self->databus, 0x0);
        dispatch_instruction(self, dec);
        break;
    case 3:
        write(self);
        break;
    case 4:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void ZPX_sequence(struct mos6502 *self, struct decoded dec)
{
    zeropage_indexed(self, dec, self->x);
}

static void ZPY_sequence(struct mos6502 *self, struct decoded dec)
{
    zeropage_indexed(self, dec, self->y);
}

static void INDX_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = bytowr(self->databus, 0x0);
        self->adl = self->databus + self->x;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl++, 0x0);
        read(self);
        break;
    case 4:
        self->addrbus = bytowr(self->adl, 0x0);
        self->adl = self->databus;
        read(self);
        break;
    case 5:
        self->addrbus = bytowr(self->adl, self->databus);
        dispatch_instruction(self, dec);
        break;
    case 6:
        // NOTE: some unofficial RMW opcodes use this addressing mode and
        // introduce write-delayed cycles similar to zp-indexed
        // or absolute-indexed.
        write(self);
        break;
    case 7:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void INDY_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = bytowr(self->databus, 0x0);
        self->adl = self->databus + 1;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl, 0x0);
        self->adl = self->databus + self->y;
        self->adc = self->adl < self->y;
        read(self);
        break;
    case 4:
        self->addrbus = bytowr(self->adl, self->databus);
        self->adh = self->databus + self->adc;
        dispatch_instruction(self, dec);
        break;
    case 5:
        self->addrbus = bytowr(self->adl, self->adh);
        dispatch_instruction(self, dec);
        break;
    case 6:
        // NOTE: some unofficial RMW opcodes use this addressing mode and
        // introduce write-delayed cycles similar to zp-indexed
        // or absolute-indexed.
        write(self);
        break;
    case 7:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void ABS_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = self->pc++;
        self->adl = self->databus;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl, self->databus);
        dispatch_instruction(self, dec);
        break;
    case 4:
        write(self);
        break;
    case 5:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void ABSX_sequence(struct mos6502 *self, struct decoded dec)
{
    absolute_indexed(self, dec, self->x);
}

static void ABSY_sequence(struct mos6502 *self, struct decoded dec)
{
    absolute_indexed(self, dec, self->y);
}

static void PSH_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc;
        read(self);
        break;
    case 2:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void PLL_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc;
        read(self);
        break;
    case 2:
        stack_top(self);
        break;
    case 3:
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void BCH_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        if (!self->detached) {
            dispatch_instruction(self, dec);
        }
        break;
    case 2:
        self->addrbus = self->pc;
        branch_displacement(self);
        read(self);
        // NOTE: interrupt polling does not happen on this cycle!
        // branch instructions only poll on branch-not-taken and
        // branch-with-page-crossing cycles.
        self->presync = !self->adc;
        break;
    case 3:
        self->addrbus = self->pc;
        branch_carry(self);
        read(self);
        commit_operation(self);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void JSR_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->adl = self->databus;
        stack_top(self);
        break;
    case 3:
        stack_push(self, self->pc >> 8);
        break;
    case 4:
        stack_push(self, self->pc);
        break;
    case 5:
        self->addrbus = self->pc;
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void RTS_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        stack_top(self);
        break;
    case 3:
        stack_pop(self);
        break;
    case 4:
        self->adl = self->databus;
        stack_pop(self);
        dispatch_instruction(self, dec);
        break;
    case 5:
        self->addrbus = self->pc++;
        read(self);
        commit_operation(self);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void JABS_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = self->pc++;
        self->adl = self->databus;
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void JIND_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = self->pc++;
        self->adl = self->databus;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl++, self->databus);
        self->adh = self->databus;
        read(self);
        break;
    case 4:
        self->addrbus = bytowr(self->adl, self->adh);
        self->adl = self->databus;
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void BRK_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc;
        read(self);
        if (!service_interrupt(self)) {
            ++self->pc;
        }
        break;
    case 2:
        stack_push(self, self->pc >> 8);
        break;
    case 3:
        stack_push(self, self->pc);
        break;
    case 4:
        stack_push(self, get_p(self, service_interrupt(self)));
        break;
    case 5:
        // NOTE: higher priority interrupts can hijack this break sequence if
        // latched in by this cycle.
        poll_interrupts(self);
        self->addrbus = interrupt_vector(self);
        read(self);
        break;
    case 6:
        self->addrbus = interrupt_vector(self) + 1;
        self->adl = self->databus;
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void RTI_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        stack_top(self);
        break;
    case 3:
        stack_pop(self);
        break;
    case 4:
        set_p(self, self->databus);
        stack_pop(self);
        break;
    case 5:
        self->adl = self->databus;
        stack_pop(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

//
// Unofficial Addressing Modes
//

static void JAM_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc;
        read(self);
        break;
    case 2:
        dispatch_instruction(self, dec);
        break;
    case 3:
    case 4:
        break;
    case 5:
        // NOTE: forever instructions have 5 time states (T0-T4)
        // so use T5 to rewind time back to T4, jamming the processor.
        // http://visual6502.org/wiki/index.php?title=6502_Timing_States#Forever_Instructions
        --self->t;
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

//
// Address Mode Dispatch
//

static void dispatch_addrmode(struct mos6502 *self, struct decoded dec)
{
    assert(0 < self->t && self->t < MaxCycleCount);

    switch (dec.mode) {
#define X(s, b, n, p, ...) case AM_ENUM(s): s##_sequence(self, dec); break;
        DEC_ADDRMODE_X
#undef X
    default:
        assert(((void)"BAD ADDRMODE DISPATCH", false));
        break;
    }
}

//
// Public Interface
//

// NOTE: all official opcodes max out at 7 cycles but a handful of
// unofficial RMW opcodes have 8 cycles when using indirect-addressing modes.
const int MaxCycleCount = 8;

void cpu_powerup(struct mos6502 *self)
{
    assert(self != NULL);

    // NOTE: Initialize physical lines and control flags to known state
    self->signal.irq = self->signal.nmi = self->signal.res =
        self->signal.rw = self->detached = true;
    self->signal.rdy = self->signal.sync = self->bflt = self->presync = false;

    // NOTE: initialize internal registers to known state
    self->pc = self->a = self->s = self->x = self->y =
        self->t = self->opc = self->adl = self->adh = self->adc = 0;
    set_p(self, 0x34);

    // TODO: simulate res held low on startup to engage reset sequence.
    self->irq = self->nmi = NIS_CLEAR;
    self->res = NIS_PENDING;
}

int cpu_cycle(struct mos6502 *self)
{
    assert(self != NULL);

    if (!self->signal.rdy) return 0;

    if (reset_held(self)) return 1;

    if (self->presync) {
        self->presync = false;
        self->t = PreFetch;
    }
    if (self->bflt) {
        self->bflt = false;
    }

    latch_interrupts(self);
    if (++self->t == 0) {
        // NOTE: T0 is always an opcode fetch
        self->signal.sync = true;
        self->addrbus = self->pc;
        read(self);
        if (service_interrupt(self)) {
            self->opc = BrkOpcode;
        } else {
            self->opc = self->databus;
            ++self->pc;
        }
        self->addrinst = self->addrbus;
    } else {
        self->signal.sync = false;
        dispatch_addrmode(self, Decode[self->opc]);
    }
    check_interrupts(self);
    return 1;
}

bool cpu_jammed(const struct mos6502 *self)
{
    return self->t == 4 && Decode[self->opc].mode == AM_JAM;
}

void cpu_snapshot(const struct mos6502 *self, struct console_state *snapshot)
{
    assert(self != NULL);
    assert(snapshot != NULL);

    snapshot->cpu.program_counter = self->pc;
    snapshot->cpu.accumulator = self->a;
    snapshot->cpu.stack_pointer = self->s;
    snapshot->cpu.status = get_p(self, false);
    snapshot->cpu.xindex = self->x;
    snapshot->cpu.yindex = self->y;

    snapshot->datapath.addressbus = self->addrbus;
    snapshot->datapath.addrlow_latch = self->adl;
    snapshot->datapath.addrhigh_latch = self->adh;
    snapshot->datapath.addrcarry_latch = self->adc;
    snapshot->datapath.busfault = self->bflt;
    snapshot->datapath.current_instruction = self->addrinst;
    snapshot->datapath.databus = self->databus;
    snapshot->datapath.exec_cycle = self->t;
    snapshot->datapath.instdone = self->presync;
    snapshot->datapath.irq = self->irq;
    snapshot->datapath.jammed = cpu_jammed(self);
    snapshot->datapath.nmi = self->nmi;
    snapshot->datapath.res = self->res;

    snapshot->lines.irq = self->signal.irq;
    snapshot->lines.nmi = self->signal.nmi;
    snapshot->lines.readwrite = self->signal.rw;
    snapshot->lines.ready = self->signal.rdy;
    snapshot->lines.reset = self->signal.res;
    snapshot->lines.sync = self->signal.sync;
}

cpu_ctx *cpu_peek_start(struct mos6502 *self)
{
    assert(self != NULL);

    cpu_ctx *const ctx = capture(self);
    // NOTE: set to read-only and reset all signals to ready cpu
    if (!self->detached) {
        detach(self);
    }
    self->irq = self->nmi = self->res = NIS_CLEAR;
    self->signal.irq = self->signal.nmi = self->signal.res =
        self->signal.rdy = true;
    return ctx;
}

struct cpu_peekresult cpu_peek(struct mos6502 *self, uint16_t addr)
{
    assert(self != NULL);

    self->presync = true;
    self->pc = addr;
    cpu_cycle(self);
    struct cpu_peekresult result = {.mode = Decode[self->opc].mode};
    // NOTE: can't run the cpu to peek JAM or it'll jam the cpu!
    // Fortunately all we need is the addressing mode so return that.
    if (result.mode == AM_JAM) return result;

    do {
        cpu_cycle(self);
        if (self->t == 2 && result.mode == AM_INDY) {
            result.interaddr = bytowr(self->databus, 0x0);
        }
        else if (self->t == 3) {
            if (result.mode == AM_INDX) {
                result.interaddr = self->addrbus;
            } else if (result.mode == AM_INDY) {
                result.interaddr = bytowr(result.interaddr, self->databus);
            }
        }
    } while (!self->presync);
    result.finaladdr = result.mode == AM_BCH || result.mode == AM_JIND
                        ? self->pc
                        : self->addrbus;
    result.data = self->databus;
    result.busfault = self->bflt;
    return result;
}

void cpu_peek_end(struct mos6502 *self, cpu_ctx *ctx)
{
    assert(self != NULL);
    assert(ctx != NULL);

    restore(self, ctx);
    if (!ctx->cpu.detached) {
        attach(self);
    }
}

//
//  cpu.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#include "cpu.h"

#include "bus.h"
#include "bytes.h"
#include "snapshot.h"

#include <assert.h>
#include <stddef.h>

//
// MARK: - State Management
//

static void detach(struct aldo_mos6502 *self)
{
    // TODO: adjust any bus devices with side-effect reads
    self->detached = true;
}

static void attach(struct aldo_mos6502 *self)
{
    // TODO: adjust any bus devices with side-effect reads
    self->detached = false;
}

//
// MARK: - Internal Operations
//

static void read(struct aldo_mos6502 *self)
{
    self->signal.rw = true;
    self->bflt = !aldo_bus_read(self->mbus, self->addrbus, &self->databus);
}

static void write(struct aldo_mos6502 *self)
{
    if (self->detached) {
        read(self);
    } else {
        self->signal.rw = false;
        self->bflt = !aldo_bus_write(self->mbus, self->addrbus, self->databus);
    }
}

static uint8_t get_p(const struct aldo_mos6502 *self, bool interrupt)
{
    return (uint8_t)
        (self->p.c
         | self->p.z << 1
         | self->p.i << 2
         | self->p.d << 3
         | !interrupt << 4  // B bit is 0 if interrupt, 1 otherwise
         | 1 << 5           // Unused bit is always set
         | self->p.v << 6
         | self->p.n << 7);
}

static void set_p(struct aldo_mos6502 *self, uint8_t p)
{
    self->p.c = p & 0x1;
    self->p.z = p & 0x2;
    self->p.i = p & 0x4;
    self->p.d = p & 0x8;
    // NOTE: skip B and unused flags, they cannot be set explicitly
    self->p.v = p & 0x40;
    self->p.n = p & 0x80;
}

static void update_z(struct aldo_mos6502 *self, uint8_t d)
{
    self->p.z = d == 0;
}

static bool bcd_mode(struct aldo_mos6502 *self)
{
    return self->bcd && self->p.d;
}

// NOTE: signed overflow happens when positive + positive = negative
// or negative + negative = positive, i.e. the sign of A does not match
// the sign of S and the sign of B does not match the sign of S:
// (Sign A ^ Sign S) & (Sign B ^ Sign S) => (A ^ S) & (B ^ S) & SignMask
static void update_v(struct aldo_mos6502 *self, uint8_t s, uint8_t a,
                     uint8_t b)
{
    self->p.v = (a ^ s) & (b ^ s) & 0x80;
}

static void update_n(struct aldo_mos6502 *self, uint8_t d)
{
    self->p.n = d & 0x80;
}

//
// MARK: - Interrupt Detection
//

// NOTE: check interrupt lines (active low) on ϕ2 to
// initiate interrupt detection.
static void check_interrupts(struct aldo_mos6502 *self)
{
    // NOTE: serviced state is only for assisting in nmi edge detection
    assert(self->rst != ALDO_SIG_SERVICED);
    assert(self->irq != ALDO_SIG_SERVICED);

    if (!self->signal.rst && self->rst == ALDO_SIG_CLEAR) {
        self->rst = ALDO_SIG_DETECTED;
    }

    // NOTE: final cycle of BRK sequence holds interrupt latching low,
    // delaying detection of any still-active interrupt signals by one cycle
    // (except RST which is all-powerful).
    if (self->opc == Aldo_BrkOpcode && self->t == 6) return;

    if (self->signal.nmi) {
        if (self->nmi == ALDO_SIG_SERVICED) {
            self->nmi = ALDO_SIG_CLEAR;
        }
    } else if (self->nmi == ALDO_SIG_CLEAR) {
        self->nmi = ALDO_SIG_DETECTED;
    }

    if (!self->signal.irq && self->irq == ALDO_SIG_CLEAR) {
        self->irq = ALDO_SIG_DETECTED;
    }
}

// NOTE: check persistence of interrupt lines (active low) on ϕ1
// of following cycle to latch interrupt detection and make polling possible.
static void latch_interrupts(struct aldo_mos6502 *self)
{
    // NOTE: rst is level-detected but unlike irq it takes effect at
    // end of this cycle's ϕ2 so it moves directly from pending in this cycle
    // to committed in the next cycle.
    if (self->signal.rst) {
        if (self->rst == ALDO_SIG_DETECTED) {
            self->rst = ALDO_SIG_CLEAR;
        }
    } else if (self->rst == ALDO_SIG_DETECTED) {
        self->rst = ALDO_SIG_PENDING;
    }

    // NOTE: nmi is edge-detected so once it has latched in it remains
    // set until serviced by a handler or reset.
    if (self->signal.nmi) {
        if (self->nmi == ALDO_SIG_DETECTED) {
            self->nmi = ALDO_SIG_CLEAR;
        }
    } else if (self->nmi == ALDO_SIG_DETECTED) {
        self->nmi = ALDO_SIG_PENDING;
    }

    // NOTE: irq is level-detected so must remain low until polled
    // or the interrupt is lost.
    if (self->signal.irq) {
        if (self->irq == ALDO_SIG_DETECTED || self->irq == ALDO_SIG_PENDING) {
            self->irq = ALDO_SIG_CLEAR;
        }
    } else if (self->irq == ALDO_SIG_DETECTED) {
        self->irq = ALDO_SIG_PENDING;
    }
}

static void poll_interrupts(struct aldo_mos6502 *self)
{
    if (self->detached) return;

    if (self->nmi == ALDO_SIG_PENDING) {
        self->nmi = ALDO_SIG_COMMITTED;
    }

    if (self->irq == ALDO_SIG_PENDING && !self->p.i) {
        self->irq = ALDO_SIG_COMMITTED;
    }
}

static bool service_interrupt(struct aldo_mos6502 *self)
{
    return self->rst == ALDO_SIG_COMMITTED
            || self->nmi == ALDO_SIG_COMMITTED
            || self->irq == ALDO_SIG_COMMITTED;
}

static uint16_t interrupt_vector(struct aldo_mos6502 *self)
{
    if (self->rst == ALDO_SIG_COMMITTED) return ALDO_CPU_VECTOR_RST;
    if (self->nmi == ALDO_SIG_COMMITTED) return ALDO_CPU_VECTOR_NMI;
    return ALDO_CPU_VECTOR_IRQ;
}

// NOTE: rst takes effect after two cycles and immediately resets/halts
// the cpu until the rst line goes high again (this isn't strictly true
// but the real cpu complexity isn't necessary here).
static bool reset_held(struct aldo_mos6502 *self)
{
    if (self->rst == ALDO_SIG_PENDING) {
        self->rst = ALDO_SIG_COMMITTED;
        detach(self);
        self->presync = true;
    }
    return self->rst == ALDO_SIG_COMMITTED && !self->signal.rst;
}

//
// MARK: - Instruction Execution
//

/*
 * Finish an instruction by polling for interrupts and signaling the next cycle
 * to be an opcode fetch; the order in which this is called simulates the
 * side-effects of 6502's pipelining behavior without actually emulating the
 * cycle timing, for example:

 * SEI is setting the I flag from 0 to 1, disabling interrupts; SEI is a
 * 2-cycle instruction and therefore polls for interrupts on the T1 (final)
 * cycle, however the I flag is not actually set until T0 of the *next* cycle,
 * so if interrupt-polling finds an active interrupt it will insert a BRK at
 * the end of SEI and an interrupt will fire despite following the
 * "disable interrupt" instruction; correspondingly CLI will delay an
 * interrupt for an extra instruction for the same reason;

 * committing the operation before executing the register side-effects will
 * emulate this pipeline timing with respect to interrupts without modelling
 * the actual cycle-delay of internal CPU operations.
 */
static void conditional_commit(struct aldo_mos6502 *self, bool c)
{
    // NOTE: interrupts are polled regardless of commit condition
    poll_interrupts(self);
    self->presync = c;
}

static void commit_operation(struct aldo_mos6502 *self)
{
    conditional_commit(self, true);
}

static void load_register(struct aldo_mos6502 *self, uint8_t *r, uint8_t d)
{
    *r = d;
    update_z(self, *r);
    update_n(self, *r);
}

static void store_data(struct aldo_mos6502 *self, uint8_t d)
{
    self->databus = d;
    write(self);
}

// NOTE: add with carry-in: A + B + C and subtract with carry-in: A - B - ~C,
// where ~carry indicates borrow-out, are equivalent in binary mode:
//  A - B => A + (-B) => A + 2sComplement(B) => A + (~B + 1) when C = 1;
//  C = 0 is thus a borrow-out; A + (~B + 0) => A + ~B => A - B - 1.
static void binary_add(struct aldo_mos6502 *self, uint8_t a, uint8_t b,
                       uint8_t c)
{
    int sum = a + b + c;
    self->p.c = sum & 0x100;
    uint8_t result = (uint8_t)sum;
    update_v(self, result, a, b);
    load_register(self, &self->a, result);
}

static void decimal_add(struct aldo_mos6502 *self, uint8_t alo, uint8_t blo,
                        uint8_t ahi, uint8_t bhi, bool c)
{
    int slo = alo + blo + c;
    bool chi = slo > 0x9;
    if (chi) {
        slo += 0x6;
        slo &= 0xf;
    }
    int shi = ahi + bhi + chi, shinib = shi << 4;
    // NOTE: overflow and negative are set before decimal adjustment
    update_v(self, (uint8_t)shinib, (uint8_t)(ahi << 4), (uint8_t)(bhi << 4));
    update_n(self, (uint8_t)shinib);
    self->p.c = shi > 0x9;
    if (self->p.c) {
        shi += 0x6;
        shi &= 0xf;
    }
    shinib = shi << 4;
    self->a = (uint8_t)(shinib | slo);
}

static void decimal_subtract(struct aldo_mos6502 *self, uint8_t alo,
                             uint8_t blo, uint8_t ahi, uint8_t bhi, int8_t brw)
{
    int dlo = alo - blo + brw;  // borrow is either 0 or -1
    bool brwhi = dlo < 0;
    if (brwhi) {
        dlo -= 0x6;
        dlo &= 0xf;
    }
    int dhi = ahi - bhi - brwhi;
    if (dhi < 0) {
        dhi -= 0x6;
        dhi &= 0xf;
    }
    // NOTE: bcd subtract does not adjust flags from earlier binary op
    self->a = (uint8_t)((dhi << 4) | dlo);
}

enum arithmetic_operator {
    AOP_ADD,
    AOP_SUB,
};

static void arithmetic_operation(struct aldo_mos6502 *self,
                                 enum arithmetic_operator op, uint8_t b)
{
    uint8_t a = self->a;
    bool c = self->p.c;
    // NOTE: even in BCD mode some flags are set as if in binary mode
    // so always do binary op regardless of BCD flag.
    binary_add(self, a, op == AOP_SUB ? ~b : b, c);

    if (!bcd_mode(self)) return;

    // NOTE: decimal mode is 4-bit nibble arithmetic with carry/borrow
    uint8_t
        alo = a & 0xf,
        blo = b & 0xf,
        ahi = (a >> 4) & 0xf,
        bhi = (b >> 4) & 0xf;
    if (op == AOP_SUB) {
        decimal_subtract(self, alo, blo, ahi, bhi, c - 1);
    } else {
        decimal_add(self, alo, blo, ahi, bhi, c);
    }
}

// NOTE: compare is effectively R - D; modeling the subtraction as
// R + 2sComplement(D) gets us all the flags for free;
// see binary_add for why this works.
static uint8_t compare_register(struct aldo_mos6502 *self, uint8_t r,
                                uint8_t d)
{
    int cmp = r + (uint8_t)~d + 1;
    self->p.c = cmp & 0x100;
    uint8_t result = (uint8_t)cmp;
    update_z(self, result);
    update_n(self, result);
    return result;
}

static void modify_mem(struct aldo_mos6502 *self, uint8_t d)
{
    store_data(self, d);
    update_z(self, d);
    update_n(self, d);
}

enum bitdirection {
    BIT_LEFT,
    BIT_RIGHT,
};

static uint8_t bitoperation(struct aldo_mos6502 *self, struct aldo_decoded dec,
                            enum bitdirection bd, uint8_t carryin_mask)
{
    // NOTE: some unofficial shift/rotate opcodes use immediate mode
    // to operate on the accumulator.
    bool acc_operand = dec.mode == ALDO_AM_IMP || dec.mode == ALDO_AM_IMM;
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

static void stack_top(struct aldo_mos6502 *self)
{
    self->addrbus = aldo_bytowr(self->s, 0x1);
    read(self);
}

static void stack_pop(struct aldo_mos6502 *self)
{
    ++self->s;
    stack_top(self);
}

static void stack_push(struct aldo_mos6502 *self, uint8_t d)
{
    self->addrbus = aldo_bytowr(self->s--, 0x1);
    store_data(self, d);
}

// NOTE: all 6502 cycles are either a read or a write, some of them discarded
// depending on the instruction and addressing-mode timing; these extra reads
// and writes are all modeled below to help verify cycle-accurate behavior.

static bool read_delayed(struct aldo_mos6502 *self, struct aldo_decoded dec,
                         bool delay_condition)
{
    if (!delay_condition) return false;

    bool delayed;
    switch (dec.mode) {
    case ALDO_AM_INDY:
        delayed = self->t == 4;
        break;
    case ALDO_AM_ABSX:
    case ALDO_AM_ABSY:
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

static bool write_delayed(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    bool delayed;
    switch (dec.mode) {
    case ALDO_AM_ZP:
        delayed = self->t == 2;
        break;
    case ALDO_AM_ZPX:
    case ALDO_AM_ZPY:
    case ALDO_AM_ABS:
        delayed = self->t == 3;
        break;
    case ALDO_AM_ABSX:
    case ALDO_AM_ABSY:
        delayed = self->t == 4;
        break;
    case ALDO_AM_INDX:
    case ALDO_AM_INDY:
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

static void UDF_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
}

static void ADC_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    arithmetic_operation(self, AOP_ADD, self->databus);
}

static void AND_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a & self->databus);
}

static void ASL_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_LEFT, 0x0);
}

// NOTE: branch instructions do not branch if the opposite condition is TRUE;
// e.g. BCC (branch on carry clear) will NOT branch if carry is SET;
// i.e. NOT(Condition) => commit instruction.

static void BCC_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, self->p.c);
}

static void BCS_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, !self->p.c);
}

static void BEQ_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, !self->p.z);
}

static void BIT_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    update_z(self, self->a & self->databus);
    self->p.v = self->databus & 0x40;
    update_n(self, self->databus);
}

static void BMI_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, !self->p.n);
}

static void BNE_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, self->p.z);
}

static void BPL_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, self->p.n);
}

static void BRK_exec(struct aldo_mos6502 *self)
{
    // NOTE: rst is only cleared by its own handler;
    // nmi is serviced by its own handler (for edge detection)
    // but cleared by all others;
    // irq is cleared by all handlers.
    if (self->rst == ALDO_SIG_COMMITTED) {
        attach(self);
        self->rst = ALDO_SIG_CLEAR;
    }
    self->nmi = self->nmi == ALDO_SIG_COMMITTED
                ? ALDO_SIG_SERVICED
                : ALDO_SIG_CLEAR;
    self->irq = ALDO_SIG_CLEAR;
    self->p.i = true;
    self->pc = aldo_bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void BVC_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, self->p.v);
}

static void BVS_exec(struct aldo_mos6502 *self)
{
    conditional_commit(self, !self->p.v);
}

static void CLC_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.c = false;
}

static void CLD_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.d = false;
}

static void CLI_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.i = false;
}

static void CLV_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.v = false;
}

static void CMP_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    compare_register(self, self->a, self->databus);
}

static void CPX_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    compare_register(self, self->x, self->databus);
}

static void CPY_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    compare_register(self, self->y, self->databus);
}

static void DEC_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    modify_mem(self, self->databus - 1);
}

static void DEX_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->x - 1);
}

static void DEY_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->y, self->y - 1);
}

static void EOR_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a ^ self->databus);
}

static void INC_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    modify_mem(self, self->databus + 1);
}

static void INX_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->x + 1);
}

static void INY_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->y, self->y + 1);
}

static void JMP_exec(struct aldo_mos6502 *self)
{
    self->pc = aldo_bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void JSR_exec(struct aldo_mos6502 *self)
{
    self->pc = aldo_bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void LDA_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->databus);
}

static void LDX_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->x, self->databus);
}

static void LDY_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->y, self->databus);
}

static void LSR_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_RIGHT, 0x0);
}

static void NOP_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    // NOTE: unofficial NOPs have reads triggered by
    // non-implied addressing modes.
    if (read_delayed(self, dec, self->adc)) return;
    if (dec.mode != ALDO_AM_IMP) {
        read(self);
    }
    commit_operation(self);
}

static void ORA_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a | self->databus);
}

static void PHA_exec(struct aldo_mos6502 *self)
{
    stack_push(self, self->a);
    commit_operation(self);
}

static void PHP_exec(struct aldo_mos6502 *self)
{
    stack_push(self, get_p(self, false));
    commit_operation(self);
}

static void PLA_exec(struct aldo_mos6502 *self)
{
    stack_pop(self);
    commit_operation(self);
    load_register(self, &self->a, self->databus);
}

static void PLP_exec(struct aldo_mos6502 *self)
{
    stack_pop(self);
    commit_operation(self);
    set_p(self, self->databus);
}

static void ROL_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_LEFT, self->p.c);
}

static void ROR_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    bitoperation(self, dec, BIT_RIGHT, (uint8_t)(self->p.c << 7));
}

static void RTI_exec(struct aldo_mos6502 *self)
{
    self->pc = aldo_bytowr(self->adl, self->databus);
    commit_operation(self);
}

static void RTS_exec(struct aldo_mos6502 *self)
{
    self->pc = aldo_bytowr(self->adl, self->databus);
}

static void SBC_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    arithmetic_operation(self, AOP_SUB, self->databus);
}

static void SEC_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.c = true;
}

static void SED_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.d = true;
}

static void SEI_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    self->p.i = true;
}

static void STA_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->a);
    commit_operation(self);
}

static void STX_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->x);
    commit_operation(self);
}

static void STY_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->y);
    commit_operation(self);
}

static void TAX_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->a);
}

static void TAY_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->y, self->a);
}

static void TSX_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->x, self->s);
}

static void TXA_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->a, self->x);
}

static void TXS_exec(struct aldo_mos6502 *self)
{
    self->s = self->x;
    commit_operation(self);
}

static void TYA_exec(struct aldo_mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->a, self->y);
}

//
// MARK: - Unofficial Instructions
//

// NOTE: magic constant that interferes with accumulator varies based on
// chip manufacture, temperature, state of chip control signals,
// and maybe other unknown factors;
// https://csdb.dk/release/?id=212346 recommends using EE.
static const uint8_t Magic = 0xee;

static void store_unstable_addresshigh(struct aldo_mos6502 *self, uint8_t d)
{
    // NOTE: if addr carry, +1 has already been stored into adh
    uint8_t adrhi = (uint8_t)(self->adh + !self->adc);
    d &= adrhi;
    // NOTE: on page-cross boundary this *occasionally* throws the calculated
    // value into ADDR_HI; emulate that behavior consistently to emphasize the
    // instability of these instructions.
    if (self->adc) {
        self->adh = d;
        self->addrbus = aldo_bytowr(self->adl, self->adh);
    }
    store_data(self, d);
}

static void ALR_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a & self->databus);
    bitoperation(self, dec, BIT_RIGHT, 0x0);
}

static void ANC_exec(struct aldo_mos6502 *self)
{
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->a & self->databus);
    self->p.c = self->a & 0x80;
}

static void ANE_exec(struct aldo_mos6502 *self)
{
    read(self);
    commit_operation(self);
    load_register(self, &self->a, (self->a | Magic) & self->x & self->databus);
}

static void ARR_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    read(self);
    commit_operation(self);
    /*
     * Not sure this is entirely correct, community docs do not
     * completely agree on exact sequence of side-effects, but the main
     * operations are AND + ROR while the invocation of the adder
     * causes the overflow and carry flags to act strangely;
     * https://csdb.dk/release/?id=212346 has the clearest description:
     *   A := A AND operand
     *   set overflow to xor of bit 7 and bit 6 of A (side-effect of adder)
     *   set carry to bit 7 of A (side-effect of adder)
     *       but hold the value until after rotate
     *   rotate A right but leave carry unaffected
     *       implemented as a standard ROR and then
     *       setting carry to held value from ADD/ADC step
     */
    uint8_t and_result = self->a & self->databus;
    load_register(self, &self->a, and_result);
    self->p.v = aldo_getbit(self->a, 7) ^ aldo_getbit(self->a, 6);
    bool c = self->a & 0x80;
    bitoperation(self, dec, BIT_RIGHT, (uint8_t)(self->p.c << 7));
    self->p.c = c;

    if (!bcd_mode(self)) return;

    /*
     * Once again https://csdb.dk/release/?id=212346 has the best
     * description of how BCD affects the final result:
     *   if low nibble of AND result + lsb of low nibble > 0x5,
     *       adjust A by 0x6 but throw away carry
     *   if high nibble of AND result + lsb of high nibble > 0x50,
     *       adjust A by 0x60 and set carry flag
     */
    if ((and_result & 0xf) + (and_result & 0x1) > 0x5) {
        self->a = (uint8_t)((self->a & 0xf0) | ((self->a + 0x6) & 0xf));
    }
    if ((and_result & 0xf0) + (and_result & 0x10) > 0x50) {
        self->a = (uint8_t)(((self->a + 0x60) & 0xf0) | (self->a & 0xf));
        self->p.c = true;
    }
}

static void DCP_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    uint8_t d = self->databus - 1;
    modify_mem(self, d);
    compare_register(self, self->a, d);
}

static void ISC_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    uint8_t d = self->databus + 1;
    modify_mem(self, d);
    arithmetic_operation(self, AOP_SUB, d);
}

static void JAM_exec(struct aldo_mos6502 *self)
{
    self->databus = 0xff;
    self->addrbus = aldo_bytowr(0xff, 0xff);
}

static void LAS_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    self->s &= self->databus;
    load_register(self, &self->a, self->s);
    load_register(self, &self->x, self->s);
}

static void LAX_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    commit_operation(self);
    load_register(self, &self->a, self->databus);
    load_register(self, &self->x, self->databus);
}

static void LXA_exec(struct aldo_mos6502 *self)
{
    read(self);
    commit_operation(self);
    uint8_t d = (self->a | Magic) & self->databus;
    load_register(self, &self->a, d);
    load_register(self, &self->x, d);
}

static void RLA_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    uint8_t d = bitoperation(self, dec, BIT_LEFT, self->p.c);
    load_register(self, &self->a, self->a & d);
}

static void RRA_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    uint8_t d = bitoperation(self, dec, BIT_RIGHT, (uint8_t)(self->p.c << 7));
    arithmetic_operation(self, AOP_ADD, d);
}

static void SAX_exec(struct aldo_mos6502 *self)
{
    store_data(self, self->a & self->x);
    commit_operation(self);
}

static void SBX_exec(struct aldo_mos6502 *self)
{
    read(self);
    commit_operation(self);
    uint8_t cmp = compare_register(self, self->a & self->x, self->databus);
    load_register(self, &self->x, cmp);
}

static void SHA_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_unstable_addresshigh(self, self->a & self->x);
    commit_operation(self);
}

static void SHX_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_unstable_addresshigh(self, self->x);
    commit_operation(self);
}

static void SHY_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_unstable_addresshigh(self, self->y);
    commit_operation(self);
}

static void SLO_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    uint8_t d = bitoperation(self, dec, BIT_LEFT, 0x0);
    load_register(self, &self->a, self->a | d);
}

static void SRE_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true) || write_delayed(self, dec)) return;
    commit_operation(self);
    uint8_t d = bitoperation(self, dec, BIT_RIGHT, 0x0);
    load_register(self, &self->a, self->a ^ d);
}

static void TAS_exec(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    self->s = self->a & self->x;
    store_unstable_addresshigh(self, self->s);
    commit_operation(self);
}

//
// MARK: - Instruction Dispatch
//

static void dispatch_instruction(struct aldo_mos6502 *self,
                                 struct aldo_decoded dec)
{
    switch (dec.instruction) {
#define X(s, d, f, ...) case ALDO_IN_LBL(s): s##_exec(__VA_ARGS__); break;
        ALDO_DEC_INST_X
#undef X
    default:
        assert(((void)"BAD INSTRUCTION DISPATCH", false));
        break;
    }
}

//
// MARK: - Addressing Mode Sequences
//

/*
 * Addressing sequences approximate the cycle-by-cycle behavior of the
 * 6502 processor; each cycle generally breaks down as follows:
 * 1. put current cycle's address on the addressbus (roughly ϕ1)
 * 2. perform internal addressing operations for next cycle
 *    (latching previous databus contents, adding address offsets, etc.)
 * 3. execute read/write/instruction, either reading data to databus
 *    or writing data from databus (roughly ϕ2)
 * Note that the actual 6502 executes instruction side-effects on T0/T1 of the
 * next instruction but we execute all side-effects within the current
 * instruction sequence (generally the last cycle); in other words we don't
 * emulate the 6502's simple pipelining.
 */

#define BAD_ADDR_SEQ assert(((void)"BAD ADDRMODE SEQUENCE", false))

static void zeropage_indexed(struct aldo_mos6502 *self,
                             struct aldo_decoded dec, uint8_t index)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = aldo_bytowr(self->databus, 0x0);
        self->adl = self->databus + index;
        read(self);
        break;
    case 3:
        self->addrbus = aldo_bytowr(self->adl, 0x0);
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

static void absolute_indexed(struct aldo_mos6502 *self,
                             struct aldo_decoded dec, uint8_t index)
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
        self->addrbus = aldo_bytowr(self->adl, self->databus);
        self->adh = self->databus + self->adc;
        dispatch_instruction(self, dec);
        break;
    case 4:
        self->addrbus = aldo_bytowr(self->adl, self->adh);
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

static void branch_displacement(struct aldo_mos6502 *self)
{
    self->adl = (uint8_t)self->pc + self->databus;
    /*
     * Branch uses signed displacement so there are three overflow cases:
     *   no overflow = no adjustment to pc-high;
     *   positive overflow = carry-in to pc-high => pch + 1;
     *   negative overflow = borrow-out from pc-high => pch - 1;
     *   subtracting -overflow condition from +overflow condition results in:
     *     no overflow = 0 - 0 => pch + 0,
     *     +overflow = 1 - 0 => pch + 1,
     *     -overflow = 0 - 1 => pch - 1 => pch + 2sComplement(1) => pch + 0xff
     */
    bool
        negative_offset = self->databus & 0x80,
        positive_overflow = self->adl < self->databus && !negative_offset,
        negative_overflow = self->adl > self->databus && negative_offset;
    self->adc = positive_overflow - negative_overflow;
    self->pc = aldo_bytowr(self->adl, (uint8_t)(self->pc >> 8));
}

static void branch_carry(struct aldo_mos6502 *self)
{
    self->pc = aldo_bytowr(self->adl, (uint8_t)((self->pc >> 8) + self->adc));
}

static void IMP_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    assert(self->t == 1);

    self->addrbus = self->pc;
    read(self);
    dispatch_instruction(self, dec);
}

static void IMM_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    assert(self->t == 1);

    self->addrbus = self->pc++;
    dispatch_instruction(self, dec);
}

static void ZP_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = aldo_bytowr(self->databus, 0x0);
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

static void ZPX_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    zeropage_indexed(self, dec, self->x);
}

static void ZPY_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    zeropage_indexed(self, dec, self->y);
}

static void INDX_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = aldo_bytowr(self->databus, 0x0);
        self->adl = self->databus + self->x;
        read(self);
        break;
    case 3:
        self->addrbus = aldo_bytowr(self->adl++, 0x0);
        read(self);
        break;
    case 4:
        self->addrbus = aldo_bytowr(self->adl, 0x0);
        self->adl = self->databus;
        read(self);
        break;
    case 5:
        self->addrbus = aldo_bytowr(self->adl, self->databus);
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

static void INDY_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 2:
        self->addrbus = aldo_bytowr(self->databus, 0x0);
        self->adl = self->databus + 1;
        read(self);
        break;
    case 3:
        self->addrbus = aldo_bytowr(self->adl, 0x0);
        self->adl = self->databus + self->y;
        self->adc = self->adl < self->y;
        read(self);
        break;
    case 4:
        self->addrbus = aldo_bytowr(self->adl, self->databus);
        self->adh = self->databus + self->adc;
        dispatch_instruction(self, dec);
        break;
    case 5:
        self->addrbus = aldo_bytowr(self->adl, self->adh);
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

static void ABS_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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
        self->addrbus = aldo_bytowr(self->adl, self->databus);
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

static void ABSX_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    absolute_indexed(self, dec, self->x);
}

static void ABSY_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    absolute_indexed(self, dec, self->y);
}

static void PSH_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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

static void PLL_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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

static void BCH_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        // NOTE: in peek mode branches are always taken
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

static void JSR_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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
        stack_push(self, (uint8_t)(self->pc >> 8));
        break;
    case 4:
        stack_push(self, (uint8_t)self->pc);
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

static void RTS_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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

static void JABS_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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

static void JIND_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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
        self->addrbus = aldo_bytowr(self->adl++, self->databus);
        self->adh = self->databus;
        read(self);
        break;
    case 4:
        self->addrbus = aldo_bytowr(self->adl, self->adh);
        self->adl = self->databus;
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
        break;
    }
}

static void BRK_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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
        stack_push(self, (uint8_t)(self->pc >> 8));
        break;
    case 3:
        stack_push(self, (uint8_t)self->pc);
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

static void RTI_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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
// MARK: - Unofficial Addressing Modes
//

static void JAM_sequence(struct aldo_mos6502 *self, struct aldo_decoded dec)
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
// MARK: - Address Mode Dispatch
//

static void dispatch_addrmode(struct aldo_mos6502 *self,
                              struct aldo_decoded dec)
{
    assert(0 < self->t && self->t < Aldo_MaxTCycle);

    switch (dec.mode) {
#define X(s, b, n, p, ...) case ALDO_AM_LBL(s): s##_sequence(self, dec); break;
        ALDO_DEC_ADDRMODE_X
#undef X
    default:
        assert(((void)"BAD ADDRMODE DISPATCH", false));
        break;
    }
}

//
// MARK: - Public Interface
//

// NOTE: all official opcodes max out at 7 cycles but a handful of
// unofficial RMW opcodes have 8 cycles when using indirect-addressing modes;
// a full breakdown of official instruction timing states can be found at:
// http://visual6502.org/wiki/index.php?title=6502_Timing_States
const int Aldo_MaxTCycle = 8;

void aldo_cpu_powerup(struct aldo_mos6502 *self)
{
    assert(self != NULL);
    assert(self->mbus != NULL);

    // NOTE: initialize physical lines and control flags to known state
    self->signal.irq = self->signal.nmi = self->signal.rst =
        self->signal.rw = true;
    self->signal.rdy = self->signal.sync = self->bflt = self->presync =
        self->detached = false;

    // NOTE: initialize registers to known state
    self->t = self->pc = self->a = self->s = self->x = self->y = 0;
    set_p(self, 0x34);

    // TODO: simulate rst held low on startup to engage reset sequence
    self->irq = self->nmi = ALDO_SIG_CLEAR;
    self->rst = ALDO_SIG_PENDING;
}

int aldo_cpu_cycle(struct aldo_mos6502 *self)
{
    assert(self != NULL);

    // NOTE: sentinel value for cycle count denoting an imminent opcode fetch
    static const int prefetch = -1;

    if (!self->signal.rdy || reset_held(self)) return 0;

    if (self->presync) {
        self->presync = false;
        self->t = prefetch;
    }

    latch_interrupts(self);
    if (++self->t == 0) {
        // NOTE: T0 is always an opcode fetch
        self->signal.sync = true;
        self->addrbus = self->pc;
        read(self);
        if (service_interrupt(self)) {
            self->opc = Aldo_BrkOpcode;
        } else {
            self->opc = self->databus;
            ++self->pc;
        }
        self->addrinst = self->addrbus;
    } else {
        self->signal.sync = false;
        dispatch_addrmode(self, Aldo_Decode[self->opc]);
    }
    check_interrupts(self);
    return 1;
}

bool aldo_cpu_jammed(const struct aldo_mos6502 *self)
{
    return self->t == 4 && Aldo_Decode[self->opc].mode == ALDO_AM_JAM;
}

void aldo_cpu_snapshot(const struct aldo_mos6502 *self,
                       struct aldo_snapshot *snp)
{
    assert(self != NULL);
    assert(snp != NULL);

    snp->cpu.program_counter = self->pc;
    snp->cpu.accumulator = self->a;
    snp->cpu.stack_pointer = self->s;
    snp->cpu.status = get_p(self, false);
    snp->cpu.xindex = self->x;
    snp->cpu.yindex = self->y;

    snp->cpu.datapath.addressbus = self->addrbus;
    snp->cpu.datapath.addrlow_latch = self->adl;
    snp->cpu.datapath.addrhigh_latch = self->adh;
    snp->cpu.datapath.addrcarry_latch = self->adc;
    snp->cpu.datapath.busfault = self->bflt;
    snp->cpu.datapath.current_instruction = self->addrinst;
    snp->cpu.datapath.databus = self->databus;
    assert(self->t >= 0);
    snp->cpu.datapath.exec_cycle = self->t;
    snp->cpu.datapath.instdone = self->presync;
    snp->cpu.datapath.irq = self->irq;
    snp->cpu.datapath.jammed = aldo_cpu_jammed(self);
    snp->cpu.datapath.nmi = self->nmi;
    snp->cpu.datapath.opcode = self->opc;
    snp->cpu.datapath.rst = self->rst;

    snp->cpu.lines.irq = self->signal.irq;
    snp->cpu.lines.nmi = self->signal.nmi;
    snp->cpu.lines.readwrite = self->signal.rw;
    snp->cpu.lines.ready = self->signal.rdy;
    snp->cpu.lines.reset = self->signal.rst;
    snp->cpu.lines.sync = self->signal.sync;
}

struct aldo_peekresult
aldo_cpu_peek_start(struct aldo_mos6502 *restrict self,
                    struct aldo_mos6502 *restrict restore)
{
    assert(self != NULL);

    if (restore) {
        *restore = *self;
    }
    // NOTE: set to read-only and reset all signals to ready cpu
    if (!self->detached) {
        detach(self);
    }
    self->irq = self->nmi = self->rst = ALDO_SIG_CLEAR;
    self->signal.irq = self->signal.nmi = self->signal.rst =
        self->signal.rdy = true;
    return (struct aldo_peekresult){.mode = Aldo_Decode[self->opc].mode};
}

void aldo_cpu_peek(struct aldo_mos6502 *self, struct aldo_peekresult *peek)
{
    assert(self != NULL);

    // NOTE: can't run the cpu to peek JAM or it'll jam the cpu!
    // Fortunately all we need is the addressing mode so return that.
    if (peek->mode == ALDO_AM_JAM) {
        peek->done = true;
        return;
    }

    if (self->presync) {
        peek->finaladdr = peek->mode == ALDO_AM_BCH
                            || peek->mode == ALDO_AM_JIND
                            ? self->pc
                            : self->addrbus;
        peek->data = self->databus;
        peek->busfault = self->bflt;
        peek->done = true;
    } else {
        aldo_cpu_cycle(self);
        if (self->t == 2 && peek->mode == ALDO_AM_INDY) {
            peek->interaddr = aldo_bytowr(self->databus, 0x0);
        }
        else if (self->t == 3) {
            if (peek->mode == ALDO_AM_INDX) {
                peek->interaddr = self->addrbus;
            } else if (peek->mode == ALDO_AM_INDY) {
                peek->interaddr = aldo_bytowr((uint8_t)peek->interaddr,
                                               self->databus);
            }
        }
    }
}

void aldo_cpu_peek_end(struct aldo_mos6502 *restrict self,
                       struct aldo_mos6502 *restrict restore)
{
    assert(self != NULL);

    if (!restore) return;

    *self = *restore;
    if (!restore->detached) {
        attach(self);
    }
}

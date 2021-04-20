//
//  cpu.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#include "cpu.h"

#include "bytes.h"
#include "decode.h"
#include "traits.h"

#include <assert.h>
#include <stddef.h>

static const int PreFetch = -1;     // Sentinel value for cycle count
                                    // denoting an imminent opcode fetch.

static void read(struct mos6502 *self)
{
    self->signal.rw = true;

    if (self->addrbus <= CpuRamMaxAddr) {
        self->databus = self->ram[self->addrbus & CpuRamAddrMask];
        return;
    }
    if (CpuCartMinAddr <= self->addrbus && self->addrbus <= CpuCartMaxAddr) {
        self->databus = self->cart[self->addrbus & CpuCartAddrMask];
        return;
    }
    self->dflt = true;
}

static void write(struct mos6502 *self)
{
    self->signal.rw = false;

    if (self->addrbus <= CpuRamMaxAddr) {
        self->ram[self->addrbus & CpuRamAddrMask] = self->databus;
        return;
    }
    self->dflt = true;
}

static uint8_t get_p(const struct mos6502 *self, bool interrupt)
{
    uint8_t p = 0x20;       // NOTE: Unused bit is always set
    p |= self->p.c << 0;
    p |= self->p.z << 1;
    p |= self->p.i << 2;
    p |= self->p.d << 3;
    p |= !interrupt << 4;   // NOTE: B bit is 0 if interrupt, 1 otherwise
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

    if (self->signal.nmi) {
        if (self->nmi == NIS_SERVICED) {
            self->nmi = NIS_CLEAR;
        }
    } else {
        if (self->nmi == NIS_CLEAR) {
            self->nmi = NIS_DETECTED;
        }
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
    if (self->nmi == NIS_PENDING) {
        self->nmi = NIS_COMMITTED;
    }

    if (self->irq == NIS_PENDING && !self->p.i) {
        self->irq = NIS_COMMITTED;
    }
}

static bool service_interrupt(struct mos6502 *self)
{
    return self->nmi == NIS_COMMITTED || self->irq == NIS_COMMITTED;
}

static uint16_t interrupt_vector(struct mos6502 *self)
{
    if (self->res == NIS_COMMITTED) return ResetVector;
    if (self->nmi == NIS_COMMITTED) return NmiVector;
    return IrqVector;
}

// NOTE: res takes effect after two cycles and immediately resets/halts
// the cpu until the res line goes high again (this isn't strictly true
// but the real cpu complexity isn't necessary here).
static bool reset_held(struct mos6502 *self)
{
    if (self->res == NIS_PENDING) {
        self->res = NIS_COMMITTED;
    }
    if (self->res == NIS_COMMITTED) {
        if (self->signal.res) {
            // TODO: fake the execution of the RES sequence
            self->addrbus = ResetVector;
            read(self);
            self->adl = self->databus;
            self->addrbus = ResetVector + 1;
            read(self);
            self->adh = self->databus;
            self->pc = bytowr(self->adl, self->adh);
            self->p.i = true;
            // NOTE: Reset runs through same sequence as BRK/IRQ
            // so the cpu does 3 phantom stack pushes;
            // on powerup this would result in $00 - $3 = $FD.
            self->s -= 3;
            self->presync = true;
            self->irq = self->nmi = self->res = NIS_CLEAR;
        } else {
            self->t = 0;
            return true;
        }
    }
    return false;
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

// NOTE: compute ones- or twos-complement of D, including any carry-out
static uint16_t complement(uint8_t d, bool twos)
{
    return (uint8_t)~d + twos;
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

// NOTE: A + B; B is u16 as it may contain a carry-out
static void arithmetic_sum(struct mos6502 *self, uint16_t b)
{
    commit_operation(self);
    const uint16_t sum = self->a + b;
    self->p.c = sum >> 8;
    update_v(self, sum, self->a, b);
    load_register(self, &self->a, sum);
}

// NOTE: compare is effectively R - D; modeling the subtraction as
// R + 2sComplement(D) gets us all the flags for free;
// see SBC_exec for why this works.
static void compare_register(struct mos6502 *self, uint8_t r)
{
    read(self);
    commit_operation(self);
    const uint16_t cmp = r + complement(self->databus, true);
    self->p.c = cmp >> 8;
    update_z(self, cmp);
    update_n(self, cmp);
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

static void bitoperation(struct mos6502 *self, struct decoded dec,
                         enum bitdirection bd, uint8_t carryin_mask)
{
    commit_operation(self);
    uint8_t d = dec.mode == AM_IMP ? self->a : self->databus;
    if (bd == BIT_LEFT) {
        self->p.c = d & 0x80;
        d <<= 1;
    } else {
        self->p.c = d & 0x1;
        d >>= 1;
    }
    d |= carryin_mask;

    if (dec.mode == AM_IMP) {
        load_register(self, &self->a, d);
    } else {
        modify_mem(self, d);
    }
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
    default:
        delayed = false;
        break;
    }
    if (delayed) {
        read(self);
    }
    return delayed;
}

static void UNK_exec(struct mos6502 *self)
{
    commit_operation(self);
}

// NOTE: add with carry-in; A + D + C
static void ADC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    arithmetic_sum(self, self->databus + self->p.c);
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
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_LEFT, 0x0);
}

// NOTE: branch instructions do not branch if the opposite condition is TRUE;
// e.g. BCC (branch on carry clear) will not branch if carry is SET;
// i.e. BranchOn(Condition) => presync = NOT(Condition).

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
    self->p.i = true;
    self->nmi = self->nmi == NIS_COMMITTED ? NIS_SERVICED : NIS_CLEAR;
    self->irq = self->res = NIS_CLEAR;
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
    compare_register(self, self->a);
}

static void CPX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    compare_register(self, self->x);
}

static void CPY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    compare_register(self, self->y);
}

static void DEC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
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
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
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
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_RIGHT, 0x0);
}

static void NOP_exec(struct mos6502 *self)
{
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
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_LEFT, self->p.c);
}

static void ROR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_RIGHT, self->p.c << 7);
}

static void RTI_exec(struct mos6502 *self)
{
    self->presync = true;
}

static void RTS_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->adl, self->databus);
}

// NOTE: subtract with carry-in; A - D - ~C, where ~carry indicates borrow-out:
// A - D => A + (-D) => A + 2sComplement(D) => A + (~D + 1) when C = 1;
// C = 0 is thus a borrow-out; A + (~D + 0) => A + ~D => A - D - 1.
static void SBC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    arithmetic_sum(self, complement(self->databus, self->p.c));
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
    commit_operation(self);
    self->s = self->x;
}

static void TYA_exec(struct mos6502 *self)
{
    commit_operation(self);
    load_register(self, &self->a, self->y);
}

static void dispatch_instruction(struct mos6502 *self, struct decoded dec)
{
    switch (dec.instruction) {
#define X(s, ...) case IN_ENUM(s): s##_exec(__VA_ARGS__); break;
        DEC_INST_X
#undef X
    default:
        assert(((void)"BAD INSTRUCTION DISPATCH", false));
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
// Note that the actual 6502 executes instruction side-effects on T0 of the
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
    // -overlow = 0 - 1 => pch - 1 => pch + 2sComplement(1) => pch + 0xff.
    const bool negative_offset = self->databus & 0x80,
               positive_overflow = self->adl < self->databus
                                   && !negative_offset,
               negative_overflow = self->adl > self->databus
                                   && negative_offset;
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
    default:
        BAD_ADDR_SEQ;
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
    default:
        BAD_ADDR_SEQ;
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
    }
}

static void BCH_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        dispatch_instruction(self, dec);
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
    }
}

static void RTI_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void dispatch_addrmode(struct mos6502 *self, struct decoded dec)
{
    assert(0 < self->t && self->t < MaxCycleCount);

    switch (dec.mode) {
#define X(s, b, ...) case AM_ENUM(s): s##_sequence(self, dec); break;
        DEC_ADDRMODE_X
#undef X
    default:
        assert(((void)"BAD ADDRMODE DISPATCH", false));
    }
}

//
// Public Interface
//

void cpu_powerup(struct mos6502 *self)
{
    assert(self != NULL);

    self->a = self->x = self->y = self->s = 0;
    // NOTE: set B, I, and unused flags high
    set_p(self, 0x34);

    // NOTE: Interrupts are inverted, high means no interrupt; rw high is read;
    // presync flag is true to put the cpu into instruction-prefetch state.
    // TODO: i think reset can make many of these initializations obsolete
    self->signal.irq = self->signal.nmi = self->signal.res = self->signal.rw =
        self->presync = true;
    self->signal.rdy = self->signal.sync = self->dflt = false;
    self->irq = self->nmi = self->res = NIS_CLEAR;

    // NOTE: all other cpu elements are indeterminate on powerup
}

void cpu_reset(struct mos6502 *self)
{
    assert(self != NULL);

    // NOTE: hold reset low and clock the cpu until reset state is entered
    self->signal.res = false;
    do {
        cpu_cycle(self);
    } while (self->res != NIS_COMMITTED);
    self->signal.res = true;
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
    if (self->dflt) {
        self->dflt = false;
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
    snapshot->datapath.current_instruction = self->addrinst;
    snapshot->datapath.databus = self->databus;
    snapshot->datapath.datafault = self->dflt;
    snapshot->datapath.exec_cycle = self->t;
    snapshot->datapath.instdone = self->presync;
    snapshot->datapath.irq = self->irq;
    snapshot->datapath.nmi = self->nmi;
    snapshot->datapath.opcode = self->opc;
    snapshot->datapath.res = self->res;

    snapshot->lines.irq = self->signal.irq;
    snapshot->lines.nmi = self->signal.nmi;
    snapshot->lines.readwrite = self->signal.rw;
    snapshot->lines.ready = self->signal.rdy;
    snapshot->lines.reset = self->signal.res;
    snapshot->lines.sync = self->signal.sync;
}

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
    assert(self->signal.rw);

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
    assert(!self->signal.rw);

    if (self->addrbus <= CpuRamMaxAddr) {
        self->ram[self->addrbus & CpuRamAddrMask] = self->databus;
        return;
    }
    self->dflt = true;
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
// Instruction Execution
//

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
    self->signal.rw = false;
    self->databus = d;
    write(self);
}

// NOTE: A + B; B is u16 as it may contain a carry-out
static void arithmetic_sum(struct mos6502 *self, uint16_t b)
{
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
    self->presync = true;
}

// NOTE: add with carry-in; A + D + C
static void ADC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    arithmetic_sum(self, self->databus + self->p.c);
    self->presync = true;
}

static void AND_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    load_register(self, &self->a, self->a & self->databus);
    self->presync = true;
}

static void ASL_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_LEFT, 0x0);
    self->presync = true;
}

// NOTE: branch instructions do not branch if the opposite condition is TRUE;
// e.g. BCC (branch on carry clear) will not branch if carry is SET;
// i.e. BranchOn(Condition) => presync = NOT(Condition).

static void BCC_exec(struct mos6502 *self)
{
    self->presync = self->p.c;
}

static void BCS_exec(struct mos6502 *self)
{
    self->presync = !self->p.c;
}

static void BEQ_exec(struct mos6502 *self)
{
    self->presync = !self->p.z;
}

static void BIT_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    update_z(self, self->a & self->databus);
    self->p.v = self->databus & 0x40;
    update_n(self, self->databus);
    self->presync = true;
}

static void BMI_exec(struct mos6502 *self)
{
    self->presync = !self->p.n;
}

static void BNE_exec(struct mos6502 *self)
{
    self->presync = self->p.z;
}

static void BPL_exec(struct mos6502 *self)
{
    self->presync = self->p.n;
}

static void BRK_exec(struct mos6502 *self)
{
    self->presync = true;
}

static void BVC_exec(struct mos6502 *self)
{
    self->presync = self->p.v;
}

static void BVS_exec(struct mos6502 *self)
{
    self->presync = !self->p.v;
}

static void CLC_exec(struct mos6502 *self)
{
    self->p.c = false;
    self->presync = true;
}

static void CLD_exec(struct mos6502 *self)
{
    self->p.d = false;
    self->presync = true;
}

static void CLI_exec(struct mos6502 *self)
{
    self->p.i = false;
    self->presync = true;
}

static void CLV_exec(struct mos6502 *self)
{
    self->p.v = false;
    self->presync = true;
}

static void CMP_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    compare_register(self, self->a);
    self->presync = true;
}

static void CPX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    compare_register(self, self->x);
    self->presync = true;
}

static void CPY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    compare_register(self, self->y);
    self->presync = true;
}

static void DEC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    modify_mem(self, self->databus - 1);
    self->presync = true;
}

static void DEX_exec(struct mos6502 *self)
{
    load_register(self, &self->x, self->x - 1);
    self->presync = true;
}

static void DEY_exec(struct mos6502 *self)
{
    load_register(self, &self->y, self->y - 1);
    self->presync = true;
}

static void EOR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    load_register(self, &self->a, self->a ^ self->databus);
    self->presync = true;
}

static void INC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    modify_mem(self, self->databus + 1);
    self->presync = true;
}

static void INX_exec(struct mos6502 *self)
{
    load_register(self, &self->x, self->x + 1);
    self->presync = true;
}

static void INY_exec(struct mos6502 *self)
{
    load_register(self, &self->y, self->y + 1);
    self->presync = true;
}

static void JMP_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->ada, self->databus);
    self->presync = true;
}

static void JSR_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->ada, self->databus);
    self->presync = true;
}

static void LDA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    load_register(self, &self->a, self->databus);
    self->presync = true;
}

static void LDX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    load_register(self, &self->x, self->databus);
    self->presync = true;
}

static void LDY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    load_register(self, &self->y, self->databus);
    self->presync = true;
}

static void LSR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_RIGHT, 0x0);
    self->presync = true;
}

static void NOP_exec(struct mos6502 *self)
{
    self->presync = true;
}

static void ORA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    load_register(self, &self->a, self->a | self->databus);
    self->presync = true;
}

static void PHA_exec(struct mos6502 *self)
{
    stack_push(self, self->a);
    self->presync = true;
}

static void PHP_exec(struct mos6502 *self)
{
    stack_push(self, get_p(self));
    self->presync = true;
}

static void PLA_exec(struct mos6502 *self)
{
    stack_pop(self);
    load_register(self, &self->a, self->databus);
    self->presync = true;
}

static void PLP_exec(struct mos6502 *self)
{
    stack_pop(self);
    set_p(self, self->databus);
    self->presync = true;
}

static void ROL_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_LEFT, self->p.c);
    self->presync = true;
}

static void ROR_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)
        || write_delayed(self, dec)) return;
    bitoperation(self, dec, BIT_RIGHT, self->p.c << 7);
    self->presync = true;
}

static void RTI_exec(struct mos6502 *self)
{
    self->presync = true;
}

static void RTS_exec(struct mos6502 *self)
{
    self->pc = bytowr(self->ada, self->databus);
}

// NOTE: subtract with carry-in; A - D - ~C, where ~carry indicates borrow-out:
// A - D => A + (-D) => A + 2sComplement(D) => A + (~D + 1) when C = 1;
// C = 0 is thus a borrow-out; A + (~D + 0) => A + ~D => A - D - 1.
static void SBC_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, self->adc)) return;
    read(self);
    arithmetic_sum(self, complement(self->databus, self->p.c));
    self->presync = true;
}

static void SEC_exec(struct mos6502 *self)
{
    self->p.c = true;
    self->presync = true;
}

static void SED_exec(struct mos6502 *self)
{
    self->p.d = true;
    self->presync = true;
}

static void SEI_exec(struct mos6502 *self)
{
    self->p.i = true;
    self->presync = true;
}

static void STA_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->a);
    self->presync = true;
}

static void STX_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->x);
    self->presync = true;
}

static void STY_exec(struct mos6502 *self, struct decoded dec)
{
    if (read_delayed(self, dec, true)) return;
    store_data(self, self->y);
    self->presync = true;
}

static void TAX_exec(struct mos6502 *self)
{
    load_register(self, &self->x, self->a);
    self->presync = true;
}

static void TAY_exec(struct mos6502 *self)
{
    load_register(self, &self->y, self->a);
    self->presync = true;
}

static void TSX_exec(struct mos6502 *self)
{
    load_register(self, &self->x, self->s);
    self->presync = true;
}

static void TXA_exec(struct mos6502 *self)
{
    load_register(self, &self->a, self->x);
    self->presync = true;
}

static void TXS_exec(struct mos6502 *self)
{
    self->s = self->x;
    self->presync = true;
}

static void TYA_exec(struct mos6502 *self)
{
    load_register(self, &self->a, self->y);
    self->presync = true;
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

#define BAD_ADDR_SEQ assert(((void)"BAD ADDRMODE SEQUENCE", false))

static void zeropage_indexed(struct mos6502 *self, struct decoded dec,
                             uint8_t index)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = bytowr(self->ada, 0x0);
        read(self);
        self->ada += index;
        break;
    case 3:
        self->addrbus = bytowr(self->ada, 0x0);
        dispatch_instruction(self, dec);
        break;
    case 4:
        self->signal.rw = false;
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
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = self->pc++;
        read(self);
        self->adb = self->databus;
        self->ada += index;
        self->adc = self->ada < index;
        break;
    case 3:
        self->addrbus = bytowr(self->ada, self->adb);
        dispatch_instruction(self, dec);
        self->adb += self->adc;
        break;
    case 4:
        self->addrbus = bytowr(self->ada, self->adb);
        dispatch_instruction(self, dec);
        break;
    case 5:
        self->signal.rw = false;
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
    self->adb = self->pc + self->ada;
    // NOTE: branch uses signed displacement so there are three overflow cases:
    // no overflow = no adjustment to pc-high;
    // positive overflow = carry-in to pc-high => pch + 1;
    // negative overflow = borrow-out from pc-high => pch - 1;
    // subtracting -overflow condition from +overflow condition results in:
    // no overflow = 0 - 0 => pch + 0,
    // +overflow = 1 - 0 => pch + 1,
    // -overlow = 0 - 1 => pch - 1 => pch + 2sComplement(1) => pch + 0xff.
    const bool negative_offset = self->ada & 0x80,
               positive_overflow = self->adb < self->ada && !negative_offset,
               negative_overflow = self->adb > self->ada && negative_offset;
    self->adc = positive_overflow - negative_overflow;
    self->pc = bytowr(self->adb, self->pc >> 8);
}

static void branch_carry(struct mos6502 *self)
{
    self->pc = bytowr(self->adb, (self->pc >> 8) + self->adc);
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
        self->signal.rw = false;
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
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = bytowr(self->ada, 0x0);
        read(self);
        self->ada += self->x;
        break;
    case 3:
        self->addrbus = bytowr(self->ada++, 0x0);
        read(self);
        self->adb = self->databus;
        break;
    case 4:
        self->addrbus = bytowr(self->ada, 0x0);
        read(self);
        self->ada = self->databus;
        break;
    case 5:
        self->addrbus = bytowr(self->adb, self->ada);
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
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = bytowr(self->ada++, 0x0);
        read(self);
        self->adb = self->databus;
        break;
    case 3:
        self->addrbus = bytowr(self->ada, 0x0);
        read(self);
        self->ada = self->databus;
        self->adb += self->y;
        self->adc = self->adb < self->y;
        break;
    case 4:
        self->addrbus = bytowr(self->adb, self->ada);
        dispatch_instruction(self, dec);
        self->ada += self->adc;
        break;
    case 5:
        self->addrbus = bytowr(self->adb, self->ada);
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
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->ada, self->databus);
        dispatch_instruction(self, dec);
        break;
    case 4:
        self->signal.rw = false;
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
        self->ada = self->databus;
        break;
    case 2:
        branch_displacement(self);
        self->addrbus = self->pc;
        read(self);
        self->presync = !self->adc;
        break;
    case 3:
        branch_carry(self);
        self->addrbus = self->pc;
        read(self);
        self->presync = true;
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
        self->ada = self->databus;
        break;
    case 2:
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
        self->signal.rw = true;
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
        self->ada = self->databus;
        break;
    case 4:
        stack_pop(self);
        dispatch_instruction(self, dec);
        break;
    case 5:
        self->addrbus = self->pc++;
        read(self);
        self->presync = true;
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
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = self->pc++;
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
        self->adc = self->databus;
        break;
    case 2:
        self->addrbus = self->pc++;
        read(self);
        self->adb = self->databus;
        break;
    case 3:
        self->addrbus = bytowr(self->adc++, self->adb);
        read(self);
        self->ada = self->databus;
        break;
    case 4:
        self->addrbus = bytowr(self->adc, self->adb);
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
    }
}

static void BRK_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
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
    self->signal.irq = self->signal.nmi = self->signal.res = self->signal.rw =
        self->presync = true;
    self->signal.rdy = self->signal.sync = self->dflt = false;

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
    self->t = MaxCycleCount - 1;
    self->addrbus = ResetVector;
    read(self);
    self->ada = self->databus;
    self->addrbus = ResetVector + 1;
    read(self);
    self->adb = self->databus;
    self->pc = bytowr(self->ada, self->adb);

    self->p.i = true;
    // NOTE: Reset runs through same sequence as BRK/IRQ
    // so the cpu does 3 phantom stack pushes;
    // on powerup this would result in $00 - $3 = $FD.
    self->s -= 3;

    self->presync = true;
}

int cpu_cycle(struct mos6502 *self)
{
    assert(self != NULL);

    if (!self->signal.rdy) return 0;

    if (self->presync) {
        self->presync = false;
        self->t = PreFetch;
    }
    if (self->dflt) {
        self->dflt = false;
    }

    if (++self->t == 0) {
        // NOTE: T0 is always an opcode fetch
        self->signal.sync = self->signal.rw = true;
        self->addrbus = self->pc++;
        read(self);
        self->opc = self->databus;
        self->addrinst = self->addrbus;
    } else {
        self->signal.sync = false;
        dispatch_addrmode(self, Decode[self->opc]);
    }
    return 1;
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
    snapshot->cpu.addra_latch = self->ada;
    snapshot->cpu.addrb_latch = self->adb;
    snapshot->cpu.addrc_latch = self->adc;
    snapshot->cpu.current_instruction = self->addrinst;
    snapshot->cpu.databus = self->databus;
    snapshot->cpu.exec_cycle = self->t;
    snapshot->cpu.opcode = self->opc;

    snapshot->lines.irq = self->signal.irq;
    snapshot->lines.nmi = self->signal.nmi;
    snapshot->lines.readwrite = self->signal.rw;
    snapshot->lines.ready = self->signal.rdy;
    snapshot->lines.reset = self->signal.res;
    snapshot->lines.sync = self->signal.sync;

    snapshot->cpu.datafault = self->dflt;
    snapshot->cpu.instdone = self->presync;
}

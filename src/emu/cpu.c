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

static void store_register(struct mos6502 *self, uint8_t r)
{
    self->signal.rw = false;
    self->databus = r;
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
    self->signal.rw = false;
    self->databus = d;
    write(self);
    update_z(self, d);
    update_n(self, d);
}

// NOTE: all 6502 cycles are either a read or a write, some of them discarded
// depending on the instruction and addressing-mode timing; these extra reads
// and writes are all modeled below to help verify cycle-accurate behavior.

static void UNK_exec(struct mos6502 *self)
{
    self->presync = true;
}

// NOTE: add with carry-in; A + D + C
static void ADC_exec(struct mos6502 *self)
{
    read(self);
    arithmetic_sum(self, self->databus + self->p.c);
    self->presync = true;
}

static void AND_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->a, self->a & self->databus);
    self->presync = true;
}

static void ASL_exec(struct mos6502 *self)
{
    (void)self;
}

static void BCC_exec(struct mos6502 *self)
{
    (void)self;
}

static void BCS_exec(struct mos6502 *self)
{
    (void)self;
}

static void BEQ_exec(struct mos6502 *self)
{
    (void)self;
}

static void BIT_exec(struct mos6502 *self)
{
    read(self);
    update_z(self, self->a & self->databus);
    self->p.v = self->databus & 0x40;
    update_n(self, self->databus);
    self->presync = true;
}

static void BMI_exec(struct mos6502 *self)
{
    (void)self;
}

static void BNE_exec(struct mos6502 *self)
{
    (void)self;
}

static void BPL_exec(struct mos6502 *self)
{
    (void)self;
}

static void BRK_exec(struct mos6502 *self)
{
    (void)self;
}

static void BVC_exec(struct mos6502 *self)
{
    (void)self;
}

static void BVS_exec(struct mos6502 *self)
{
    (void)self;
}

static void CLC_exec(struct mos6502 *self)
{
    read(self);
    self->p.c = false;
    self->presync = true;
}

static void CLD_exec(struct mos6502 *self)
{
    read(self);
    self->p.d = false;
    self->presync = true;
}

static void CLI_exec(struct mos6502 *self)
{
    read(self);
    self->p.i = false;
    self->presync = true;
}

static void CLV_exec(struct mos6502 *self)
{
    read(self);
    self->p.v = false;
    self->presync = true;
}

static void CMP_exec(struct mos6502 *self)
{
    compare_register(self, self->a);
    self->presync = true;
}

static void CPX_exec(struct mos6502 *self)
{
    compare_register(self, self->x);
    self->presync = true;
}

static void CPY_exec(struct mos6502 *self)
{
    compare_register(self, self->y);
    self->presync = true;
}

static void DEC_exec(struct mos6502 *self)
{
    modify_mem(self, self->databus - 1);
    self->presync = true;
}

static void DEX_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->x, self->x - 1);
    self->presync = true;
}

static void DEY_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->y, self->y - 1);
    self->presync = true;
}

static void EOR_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->a, self->a ^ self->databus);
    self->presync = true;
}

static void INC_exec(struct mos6502 *self)
{
    (void)self;
}

static void INX_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->x, self->x + 1);
    self->presync = true;
}

static void INY_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->y, self->y + 1);
    self->presync = true;
}

static void JMP_exec(struct mos6502 *self)
{
    (void)self;
}

static void JSR_exec(struct mos6502 *self)
{
    (void)self;
}

static void LDA_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->a, self->databus);
    self->presync = true;
}

static void LDX_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->x, self->databus);
    self->presync = true;
}

static void LDY_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->y, self->databus);
    self->presync = true;
}

static void LSR_exec(struct mos6502 *self)
{
    (void)self;
}

static void NOP_exec(struct mos6502 *self)
{
    read(self);
    self->presync = true;
}

static void ORA_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->a, self->a | self->databus);
    self->presync = true;
}

static void PHA_exec(struct mos6502 *self)
{
    (void)self;
}

static void PHP_exec(struct mos6502 *self)
{
    (void)self;
}

static void PLA_exec(struct mos6502 *self)
{
    (void)self;
}

static void PLP_exec(struct mos6502 *self)
{
    (void)self;
}

static void ROL_exec(struct mos6502 *self)
{
    (void)self;
}

static void ROR_exec(struct mos6502 *self)
{
    (void)self;
}

static void RTI_exec(struct mos6502 *self)
{
    (void)self;
}

static void RTS_exec(struct mos6502 *self)
{
    (void)self;
}

// NOTE: subtract with carry-in; A - D - ~C, where ~carry indicates borrow-out:
// A - D => A + (-D) => A + 2sComplement(D) => A + (~D + 1) when C = 1;
// C = 0 is thus a borrow-out; A + (~D + 0) => A + ~D => A - D - 1.
static void SBC_exec(struct mos6502 *self)
{
    read(self);
    arithmetic_sum(self, complement(self->databus, self->p.c));
    self->presync = true;
}

static void SEC_exec(struct mos6502 *self)
{
    read(self);
    self->p.c = true;
    self->presync = true;
}

static void SED_exec(struct mos6502 *self)
{
    read(self);
    self->p.d = true;
    self->presync = true;
}

static void SEI_exec(struct mos6502 *self)
{
    read(self);
    self->p.i = true;
    self->presync = true;
}

static void STA_exec(struct mos6502 *self)
{
    store_register(self, self->a);
    self->presync = true;
}

static void STX_exec(struct mos6502 *self)
{
    store_register(self, self->x);
    self->presync = true;
}

static void STY_exec(struct mos6502 *self)
{
    store_register(self, self->y);
    self->presync = true;
}

static void TAX_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->x, self->a);
    self->presync = true;
}

static void TAY_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->y, self->a);
    self->presync = true;
}

static void TSX_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->x, self->s);
    self->presync = true;
}

static void TXA_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->a, self->x);
    self->presync = true;
}

static void TXS_exec(struct mos6502 *self)
{
    read(self);
    self->s = self->x;
    self->presync = true;
}

static void TYA_exec(struct mos6502 *self)
{
    read(self);
    load_register(self, &self->a, self->y);
    self->presync = true;
}

static void dispatch_instruction(struct mos6502 *self, struct decoded dec)
{
    switch (dec.instruction) {
#define X(s) case IN_ENUM(s): s##_exec(self); break;
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

// NOTE: all read-modify-write instructions have a miswrite cycle
static bool mem_write_delayed(struct decoded dec)
{
    switch (dec.instruction) {
    case IN_ASL:
    case IN_DEC:
    case IN_INC:
    case IN_LSR:
    case IN_ROL:
    case IN_ROR:
        return true;
    default:
        return false;
    }
}

// NOTE: store and read-modify-write instructions have a dead read cycle,
// load instructions have a dead read when address high has a carry-in.
static bool mem_read_delayed(const struct mos6502 *self, struct decoded dec)
{
    switch (dec.instruction) {
    case IN_STA:
    case IN_STX:
    case IN_STY:
        return true;
    default:
        return mem_write_delayed(dec) || self->adc;
    }
}

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
        if (mem_write_delayed(dec)) {
            read(self);
        } else {
            dispatch_instruction(self, dec);
        }
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
        self->ada += index;
        self->adc = self->ada < index;
        self->adb = self->databus;
        break;
    case 3:
        self->addrbus = bytowr(self->ada, self->adb);
        if (mem_read_delayed(self, dec)) {
            read(self);
        } else {
            dispatch_instruction(self, dec);
        }
        self->adb += self->adc;
        break;
    case 4:
        self->addrbus = bytowr(self->ada, self->adb);
        if (mem_write_delayed(dec)) {
            read(self);
        } else {
            dispatch_instruction(self, dec);
        }
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

static void IMP_sequence(struct mos6502 *self, struct decoded dec)
{
    assert(self->t == 1);

    self->addrbus = self->pc;
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
        self->ada = self->databus;
        break;
    case 2:
        self->addrbus = bytowr(self->ada, 0x0);
        if (mem_write_delayed(dec)) {
            read(self);
        } else {
            dispatch_instruction(self, dec);
        }
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
        if (mem_read_delayed(self, dec)) {
            read(self);
        } else {
            dispatch_instruction(self, dec);
        }
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
        self->adb = self->databus;
        break;
    case 3:
        self->addrbus = bytowr(self->ada, self->adb);
        if (mem_write_delayed(dec)) {
            read(self);
        } else {
            dispatch_instruction(self, dec);
        }
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
    (void)self, (void)dec;
}

static void PLL_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BCH_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void JSR_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void RTS_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void JABS_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void JIND_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
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
    snapshot->cpu.addr_carry = self->adc;
    snapshot->cpu.databus = self->databus;
    snapshot->cpu.exec_cycle = self->t;
    snapshot->cpu.opcode = self->opc;
    snapshot->cpu.datafault = self->dflt;

    snapshot->lines.irq = self->signal.irq;
    snapshot->lines.nmi = self->signal.nmi;
    snapshot->lines.readwrite = self->signal.rw;
    snapshot->lines.ready = self->signal.rdy;
    snapshot->lines.reset = self->signal.res;
    snapshot->lines.sync = self->signal.sync;
}

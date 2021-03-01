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

static const int MaxCycleCount = 7,
                 PreFetch = -1;     // Sentinel value for cycle count
                                    // denoting an imminent opcode fetch.

static void read(struct mos6502 *self)
{
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

static void set_z(struct mos6502 *self, uint8_t r)
{
    self->p.z = r == 0;
}

static void set_n(struct mos6502 *self, uint8_t r)
{
    self->p.n = r & 0x80;
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
// Instruction Execution
//

static void UNK_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->idone = true;
}

static void ADC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void AND_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ASL_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BCC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BCS_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BEQ_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BIT_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BMI_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BNE_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BPL_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BRK_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BVC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void BVS_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void CLC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.c = false;
    self->idone = true;
}

static void CLD_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.d = false;
    self->idone = true;
}

static void CLI_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.i = false;
    self->idone = true;
}

static void CLV_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.v = false;
    self->idone = true;
}

static void CMP_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void CPX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void CPY_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void DEC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void DEX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    --self->x;
    set_z(self, self->x);
    set_n(self, self->x);
    self->idone = true;
}

static void DEY_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    --self->y;
    set_z(self, self->y);
    set_n(self, self->y);
    self->idone = true;
}

static void EOR_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void INC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void INX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    ++self->x;
    set_z(self, self->x);
    set_n(self, self->x);
    self->idone = true;
}

static void INY_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    ++self->y;
    set_z(self, self->y);
    set_n(self, self->y);
    self->idone = true;
}

static void JMP_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void JSR_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void LDA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void LDX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void LDY_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void LSR_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void NOP_exec(struct mos6502 *self, struct decoded dec)
{
    // NOP does nothing!
    (void)dec;
    self->idone = true;
}

static void ORA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void PHA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void PHP_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void PLA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void PLP_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ROL_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ROR_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void RTI_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void RTS_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void SBC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void SEC_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.c = true;
    self->idone = true;
}

static void SED_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.d = true;
    self->idone = true;
}

static void SEI_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->p.i = true;
    self->idone = true;
}

static void STA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void STX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void STY_exec(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void TAX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->x = self->a;
    set_z(self, self->x);
    set_n(self, self->x);
    self->idone = true;
}

static void TAY_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->y = self->a;
    set_z(self, self->y);
    set_n(self, self->y);
    self->idone = true;
}

static void TSX_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->x = self->s;
    set_z(self, self->x);
    set_n(self, self->x);
    self->idone = true;
}

static void TXA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->a = self->x;
    set_z(self, self->a);
    set_n(self, self->a);
    self->idone = true;
}

static void TXS_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->s = self->x;
    self->idone = true;
}

static void TYA_exec(struct mos6502 *self, struct decoded dec)
{
    (void)dec;
    self->a = self->y;
    set_z(self, self->a);
    set_n(self, self->a);
    self->idone = true;
}

static void dispatch_instruction(struct mos6502 *self, struct decoded dec)
{
    switch (dec.instruction) {
#define X(s) case IN_ENUM(s): s##_exec(self, dec); break;
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

static void IMP_sequence(struct mos6502 *self, struct decoded dec)
{
    assert(self->t == 1);

    // NOTE: implied mode dead read
    self->addrbus = self->pc;
    read(self);
    dispatch_instruction(self, dec);
}

static void IMM_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ZP_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ZPX_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ZPY_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void INDX_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void INDY_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ABS_sequence(struct mos6502 *self, struct decoded dec)
{
    switch (self->t) {
    case 1:
        self->addrbus = self->pc++;
        read(self);
        self->adl = self->databus;
        break;
    case 2:
        self->addrbus = self->pc++;
        read(self);
        break;
    case 3:
        self->addrbus = bytowr(self->adl, self->databus);
        read(self);
        dispatch_instruction(self, dec);
        break;
    default:
        BAD_ADDR_SEQ;
    }
}

static void ABSX_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
}

static void ABSY_sequence(struct mos6502 *self, struct decoded dec)
{
    (void)self, (void)dec;
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
    // idone flag is true to put the cpu into instruction-prefetch state.
    self->signal.irq = self->signal.nmi = self->signal.res = self->signal.rw =
        self->idone = true;
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
    self->adl = self->databus;
    self->addrbus = ResetVector + 1;
    read(self);
    self->pc = bytowr(self->adl, self->databus);

    self->p.i = true;
    // NOTE: Reset runs through same sequence as BRK/IRQ
    // so the cpu does 3 phantom stack pushes;
    // on powerup this would result in $00 - $3 = $FD.
    self->s -= 3;

    self->idone = true;
}

int cpu_cycle(struct mos6502 *self)
{
    assert(self != NULL);

    if (!self->signal.rdy) return 0;

    if (self->idone) {
        self->idone = false;
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
    if (self->signal.sync) {
        snapshot->cpu.currinst = self->addrbus;
    }
    snapshot->cpu.addrlow_latch = self->adl;
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

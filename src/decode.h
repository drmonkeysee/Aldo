//
//  decode.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#ifndef Aldo_decode_h
#define Aldo_decode_h

#include <stdbool.h>
#include <stdint.h>

// 6502 Instructions
// X(symbol, description, affected flags, instruction dispatch arguments...)
#define DEC_INST_X \
X(UDF,                              /* Undefined */ \
  "Undefined", \
  0x20, self) \
\
X(ADC,                              /* Add with carry */ \
  "Add with carry", \
  0xe3, self, dec) \
X(AND,                              /* Logical AND */ \
  "Logical AND", \
  0xa2, self, dec) \
X(ASL,                              /* Arithmetic shift left */ \
  "Arithmetic shift left", \
  0xa3, self, dec) \
\
X(BCC,                              /* Branch if carry clear */ \
  "Branch if carry clear", \
  0x20, self) \
X(BCS,                              /* Branch if carry set */ \
  "Branch if carry set", \
  0x20, self) \
X(BEQ,                              /* Branch if zero */ \
  "Branch if zero", \
  0x20, self) \
X(BIT,                              /* Test bits */ \
  "Test bits with accumulator", \
  0xe2, self, dec) \
X(BMI,                              /* Branch if negative */ \
  "Branch if negative", \
  0x20, self) \
X(BNE,                              /* Branch if not zero */ \
  "Branch if not zero", \
  0x20, self) \
X(BPL,                              /* Branch if positive */ \
  "Branch if positive", \
  0x20, self) \
X(BRK,                              /* Break */ \
  "Break", \
  0x34, self) \
X(BVC,                              /* Branch if overflow clear */ \
  "Branch if overflow clear", \
  0x20, self) \
X(BVS,                              /* Branch if overflow set */ \
  "Branch if overflow set", \
  0x20, self) \
\
X(CLC,                              /* Clear carry */ \
  "Clear carry", \
  0x21, self) \
X(CLD,                              /* Clear decimal mode */ \
  "Clear decimal mode", \
  0x28, self) \
X(CLI,                              /* Clear interrupt disable */ \
  "Clear interrupt disable", \
  0x24, self) \
X(CLV,                              /* Clear overflow */ \
  "Clear overflow", \
  0x60, self) \
X(CMP,                              /* Compare to accumulator */ \
  "Compare to accumulator", \
  0xa3, self, dec) \
X(CPX,                              /* Compare to x-index */ \
  "Compare to x-index", \
  0xa3, self, dec) \
X(CPY,                              /* Compare to y-index */ \
  "Compare to y-index", \
  0xa3, self, dec) \
\
X(DEC,                              /* Decrement memory */ \
  "Decrement memory", \
  0xa2, self, dec) \
X(DEX,                              /* Decrement x-index */ \
  "Decrement x-index", \
  0xa2, self) \
X(DEY,                              /* Decrement y-index */ \
  "Decrement y-index", \
  0xa2, self) \
\
X(EOR,                              /* Logical EXCLUSIVE OR */ \
  "Logical EXCLUSIVE OR", \
  0xa2, self, dec) \
\
X(INC,                              /* Increment memory */ \
  "Increment memory", \
  0xa2, self, dec) \
X(INX,                              /* Increment x-index */ \
  "Increment x-index", \
  0xa2, self) \
X(INY,                              /* Increment y-index */ \
  "Increment y-index", \
  0xa2, self) \
\
X(JMP,                              /* Jump */ \
  "Jump to address", \
  0x20, self) \
X(JSR,                              /* Jump to subroutine */ \
  "Jump to subroutine", \
  0x20, self) \
\
X(LDA,                              /* Load accumulator */ \
  "Load accumulator", \
  0xa2, self, dec) \
X(LDX,                              /* Load x-index */ \
  "Load x-index", \
  0xa2, self, dec) \
X(LDY,                              /* Load y-index */ \
  "Load y-index", \
  0xa2, self, dec) \
X(LSR,                              /* Logical shift right */ \
  "Logical shift right", \
  0xa3, self, dec) \
\
X(NOP,                              /* No-op */ \
  "No operation", \
  0x20, self, dec) \
\
X(ORA,                              /* Logical OR */ \
  "Logical OR", \
  0xa2, self, dec) \
\
X(PHA,                              /* Push accumulator */ \
  "Push accumulator", \
  0x20, self) \
X(PHP,                              /* Push status */ \
  "Push processor status", \
  0x20, self) \
X(PLA,                              /* Pull accumulator */ \
  "Pull accumulator", \
  0xa2, self) \
X(PLP,                              /* Pull status */ \
  "Pull processor status", \
  0xff, self) \
\
X(ROL,                              /* Rotate left */ \
  "Rotate left", \
  0xa3, self, dec) \
X(ROR,                              /* Rotate right */ \
  "Rotate left", \
  0xa3, self, dec) \
X(RTI,                              /* Return from interrupt */ \
  "Return from interrupt", \
  0xff, self) \
X(RTS,                              /* Return from subroutine */ \
  "Return from subroutine", \
  0x20, self) \
\
X(SBC,                              /* Subtract with carry */ \
  "Subtract with carry", \
  0xe3, self, dec) \
X(SEC,                              /* Set carry */ \
  "Set carry", \
  0x21, self) \
X(SED,                              /* Set decimal mode */ \
  "Set decimal mode", \
  0x28, self) \
X(SEI,                              /* Set interrupt disable */ \
  "Set interrupt disable", \
  0x24, self) \
X(STA,                              /* Store accumulator */ \
  "Store accumulator", \
  0x20, self, dec) \
X(STX,                              /* Store x-index */ \
  "Store x-index", \
  0x20, self, dec) \
X(STY,                              /* Store y-index */ \
  "Store y-index", \
  0x20, self, dec) \
\
X(TAX,                              /* Transfer accumulator to x-index */ \
  "Transfer accumulator to x-index", \
  0xa2, self) \
X(TAY,                              /* Transfer accumulator to y-index */ \
  "Transfer accumulator to y-index", \
  0xa2, self) \
X(TSX,                              /* Transfer stack pointer to x-index */ \
  "Transfer stack pointer to x-index", \
  0xa2, self) \
X(TXA,                              /* Transfer x-index to accumulator */ \
  "Transfer x-index to accumulator", \
  0xa2, self) \
X(TXS,                              /* Transfer x-index to stack pointer */ \
  "Transfer x-index to stack pointer", \
  0x20, self) \
X(TYA,                              /* Transfer y-index to accumulator */ \
  "Transfer y-index to accumulator", \
  0xa2, self) \
\
/* Unofficial Opcodes */ \
X(ALR,                              /* Logical AND and logical shift right */ \
  "AND + LSR", \
  0xa3, self, dec) \
X(ANC,                              /* Logical AND and set carry as if
                                       ASL or ROL */ \
  "AND + set carry as if ASL or ROL", \
  0xa3, self) \
X(ANE,                              /* Accumulator interferes with internal
                                       signals, logical AND with x-index and
                                       operand, load into accumulator */ \
  "AND with x-index + LDA with accumulator interference (highly unstable)", \
  0xa2, self) \
X(ARR,                              /* Logical AND and rotate right with adder
                                       side-effects interleaved between
                                       AND and ROR */ \
  "AND + ROR with adder side-effects", \
  0xe3, self, dec) \
X(DCP,                              /* Decrement memory and compare
                                       to accumulator */ \
  "DEC + CMP", \
  0xa3, self, dec) \
X(ISC,                              /* Increment memory and subtract
                                       with carry */ \
  "INC + SBC", \
  0xe3, self, dec) \
X(JAM,                              /* No instruction signal, jams the cpu */ \
  "Jams the CPU", \
  0x20, self) \
X(LAS,                              /* Logical AND with stack pointer and load
                                       into accumulator, x-index, and
                                       stack pointer */ \
  "AND with stack pointer + store stack pointer + LDA/TSX", \
  0xa2, self, dec) \
X(LAX,                              /* Load accumulator and x-index */ \
  "LDA + LDX", \
  0xa2, self, dec) \
X(LXA,                              /* Accumulator interferes with internal
                                       signals, logical AND with operand,
                                       load into accumulator and x-index */ \
  "AND + LDA/LDX with accumulator interference (highly unstable)", \
  0xa2, self) \
X(RLA,                              /* Rotate left and logical AND */ \
  "ROL + AND", \
  0xa3, self, dec) \
X(RRA,                              /* Rotate right and add with carry */ \
  "ROR + ADC", \
  0xe3, self, dec) \
X(SAX,                              /* Store logical AND of accumulator
                                       and x-index */ \
  "Store AND of accumulator and x-index", \
  0x20, self) \
X(SBX,                              /* Logical AND accumulator and x-index,
                                       compare to operand, and load into
                                       x-index */ \
  "AND accumulator and x-index + CMP + LDX", \
  0xa3, self) \
X(SHA,                              /* Store logical AND accumulator and
                                       x-index and ADDR_HI + 1 */ \
  "Store AND of accumulator, x-index, and high address byte + 1 (unstable)", \
  0x20, self, dec) \
X(SHX,                              /* Store logical AND x-index and
                                       ADDR_HI + 1 */ \
  "Store AND of x-index, and high address byte + 1 (unstable)", \
  0x20, self, dec) \
X(SHY,                              /* Store logical AND y-index and
                                       ADDR_HI + 1 */ \
  "Store AND of y-index, and high address byte + 1 (unstable)", \
  0x20, self, dec) \
X(SLO,                              /* Arithmetic shift left and logical OR */ \
  "ASL + ORA", \
  0xa3, self, dec) \
X(SRE,                              /* Logical shift right and logical
                                       EXCLUSIVE OR */ \
  "LSR + EOR", \
  0xa3, self, dec) \
X(TAS,                              /* Load logical AND accumulator and x-index
                                       into stack pointer, store logical AND
                                       accumulator and x-index and
                                       ADDR_HI + 1 */ \
  "Load AND of accumulator and x-index into stack pointer +" \
  " store AND of x-index, and high address byte + 1 (unstable)", \
  0x20, self, dec)

// Addressing Modes
// X(symbol, byte count, name, peek template, display strings...)
#define DEC_ADDRMODE_X \
X(IMP, 1, "Implied",                /* Implied */ \
  XPEEK(""), \
  "imp", "") \
X(IMM, 2, "Immediate",              /* Immediate */ \
  XPEEK(""), \
  "imm", "#$%02X") \
X(ZP, 2, "Zero-Page",               /* Zero-Page */ \
  XPEEK("= %02X", peek.data), \
  "zp", "$%02X") \
X(ZPX, 2, "Zero-Page X-Indexed",    /* Zero-Page,X */ \
  XPEEK("@ %02X = %02X", peek.finaladdr, peek.data), \
  "zp,X", "$%02X,X") \
X(ZPY, 2, "Zero-Page Y-Indexed",    /* Zero-Page,Y */ \
  XPEEK("@ %02X = %02X", peek.finaladdr, peek.data), \
  "zp,Y", "$%02X,Y") \
X(INDX, 2, "Indexed Indirect",      /* (Indirect,X) */ \
  XPEEK("@ %02X > %04X = %02X", peek.interaddr, peek.finaladdr, peek.data), \
  "(zp,X)", "($%02X,X)") \
X(INDY, 2, "Indirect Indexed",      /* (Indirect),Y */ \
  XPEEK("> %04X @ %04X = %02X", peek.interaddr, peek.finaladdr, peek.data), \
  "(zp),Y", "($%02X),Y") \
X(ABS, 3, "Absolute",               /* Absolute */ \
  XPEEK("= %02X", peek.data), \
  "abs", "$??%02X", "$%04X") \
X(ABSX, 3, "Absolute X-Indexed",    /* Absolute,X */ \
  XPEEK("@ %04X = %02X", peek.finaladdr, peek.data), \
  "abs,X", "$??%02X,X", "$%04X,X") \
X(ABSY, 3, "Absolute Y-Indexed",    /* Absolute,Y */ \
  XPEEK("@ %04X = %02X", peek.finaladdr, peek.data), \
  "abs,Y", "$??%02X,Y", "$%04X,Y") \
\
/* Stack */ \
X(PSH, 1, "Implied",                /* Push */ \
  XPEEK(""), \
  "imp", "") \
X(PLL, 1, "Implied",                /* Pull */ \
  XPEEK(""), \
  "imp", "") \
\
/* Branch */ \
X(BCH, 2, "Relative",               /* Relative branch */ \
  XPEEK("@ %04X", peek.finaladdr), \
  "rel", "%+hhd") \
\
/* Jumps */ \
X(JSR, 3, "Absolute",               /* Jump to subroutine, */ \
  XPEEK(""), \
  "abs", "$??%02X", "$%04X") \
X(RTS, 1, "Implied",                /* Return from subroutine */ \
  XPEEK(""), \
  "imp", "") \
X(JABS, 3, "Absolute",              /* Absolute jump */ \
  XPEEK(""), \
  "abs", "$??%02X", "$%04X") \
X(JIND, 3, "Indirect",              /* Indirect jump */ \
  XPEEK("> %04X", peek.finaladdr), \
  "(abs)", "($??%02X)", "($%04X)") \
\
/* Interrupts */ \
X(BRK, 1, "Implied",                /* Break, interrupt, reset */ \
  XPEEK(""), \
  "imp", "%s") \
X(RTI, 1, "Implied",                /* Return from interrupt */ \
  XPEEK(""), \
  "imp", "") \
\
/* Unofficial Modes */ \
X(JAM, 1, "Implied",                /* Jammed instruction */ \
  XPEEK(""), \
  "imp", "")

#define IN_ENUM(s) IN_##s

enum inst {
#define X(s, d, f, ...) IN_ENUM(s),
    DEC_INST_X
#undef X
};

#define AM_ENUM(s) AM_##s

enum addrmode {
#define X(s, b, p, ...) AM_ENUM(s),
    DEC_ADDRMODE_X
#undef X
};

struct decoded {
    enum inst instruction;
    enum addrmode mode;
    struct {
        int8_t count;
        bool branch_taken, page_boundary;
    } cycles;
    bool unofficial;
};

extern const int BrkOpcode;
extern const struct decoded Decode[];

#endif

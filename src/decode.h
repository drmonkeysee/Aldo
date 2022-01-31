//
//  decode.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#ifndef Aldo_decode_h
#define Aldo_decode_h

#include <stdbool.h>

// 6502 Instructions
// X(symbol, instruction dispatch arguments...)
#define DEC_INST_X \
X(UDF, self)        /* Undefined */ \
\
X(ADC, self, dec)   /* Add with carry */ \
X(AND, self, dec)   /* Logical and */ \
X(ASL, self, dec)   /* Arithmetic shift left */ \
\
X(BCC, self)        /* Branch if carry clear */ \
X(BCS, self)        /* Branch if carry set */ \
X(BEQ, self)        /* Branch if zero */ \
X(BIT, self, dec)   /* Test bits */ \
X(BMI, self)        /* Branch if negative */ \
X(BNE, self)        /* Branch if not zero */ \
X(BPL, self)        /* Branch if positive */ \
X(BRK, self)        /* Break */ \
X(BVC, self)        /* Branch if overflow clear */ \
X(BVS, self)        /* Branch if overflow set */ \
\
X(CLC, self)        /* Clear carry */ \
X(CLD, self)        /* Clear decimal */ \
X(CLI, self)        /* Clear interrupt disable */ \
X(CLV, self)        /* Clear overflow */ \
X(CMP, self, dec)   /* Compare to accumulator */ \
X(CPX, self, dec)   /* Compare to x-index */ \
X(CPY, self, dec)   /* Compare to y-index */ \
\
X(DEC, self, dec)   /* Decrement memory */ \
X(DEX, self)        /* Decrement x-index */ \
X(DEY, self)        /* Decrement y-index */ \
\
X(EOR, self, dec)   /* Logical exclusive or */ \
\
X(INC, self, dec)   /* Increment memory */ \
X(INX, self)        /* Increment x-index */ \
X(INY, self)        /* Increment y-index */ \
\
X(JMP, self)        /* Jump */ \
X(JSR, self)        /* Jump to subroutine */ \
\
X(LDA, self, dec)   /* Load accumulator */ \
X(LDX, self, dec)   /* Load x-index */ \
X(LDY, self, dec)   /* Load y-index */ \
X(LSR, self, dec)   /* Logical shift right */ \
\
X(NOP, self, dec)   /* No-op */ \
\
X(ORA, self, dec)   /* Logical or */ \
\
X(PHA, self)        /* Push accumulator */ \
X(PHP, self)        /* Push status */ \
X(PLA, self)        /* Pull accumulator */ \
X(PLP, self)        /* Pull status */ \
\
X(ROL, self, dec)   /* Rotate left */ \
X(ROR, self, dec)   /* Rotate right */ \
X(RTI, self)        /* Return from interrupt */ \
X(RTS, self)        /* Return from subroutine */ \
\
X(SBC, self, dec)   /* Subtract with carry */ \
X(SEC, self)        /* Set carry */ \
X(SED, self)        /* Set decimal */ \
X(SEI, self)        /* Set interrupt disable */ \
X(STA, self, dec)   /* Store accumulator */ \
X(STX, self, dec)   /* Store x-index */ \
X(STY, self, dec)   /* Store y-index */ \
\
X(TAX, self)        /* Transfer accumulator to x-index */ \
X(TAY, self)        /* Transfer accumulator to y-index */ \
X(TSX, self)        /* Transfer stack pointer to x-index */ \
X(TXA, self)        /* Transfer x-index to accumulator */ \
X(TXS, self)        /* Transfer x-index to stack pointer */ \
X(TYA, self)        /* Transfer y-index to accumulator */ \
\
/* Unofficial Opcodes */ \
X(ALR, self, dec)   /* Logical and and logical shift right */ \
X(ANC, self)        /* Logical and and set carry as if ASL or ROL */ \
X(ANE, self)        /* Accumulator interferes with internal signals, logical
                       and with x-index and operand, load into accumulator */ \
X(ARR, self, dec)   /* Logical and and rotate right with adder side-effects
                       interleaved between AND and ROR */ \
X(DCP, self, dec)   /* Decrement memory and compare to accumulator */ \
X(ISC, self, dec)   /* Increment memory and subtract with carry */ \
X(JAM, self)        /* No instruction signal, jams the cpu */ \
X(LAS, self, dec)   /* Logical and with stack pointer and load
                       into accumulator, x-index, and stack pointer */ \
X(LAX, self, dec)   /* Load accumulator and x-index */ \
X(LXA, self)        /* Accumulator interferes with internal signals, logical
                       and with operand, load into accumulator and x-index */ \
X(RLA, self, dec)   /* Rotate left and logical and */ \
X(RRA, self, dec)   /* Rotate right and add with carry */ \
X(SAX, self)        /* Store logical and of accumulator and x-index */ \
X(SBX, self)        /* Logical and accumulator and x-index,
                       compare to operand, and load into x-index */ \
X(SHA, self, dec)   /* Logical and accumulator and x-index and ADDR_HI + 1 */ \
X(SHX, self, dec)   /* Logical and x-index and ADDR_HI + 1 */ \
X(SLO, self, dec)   /* Arithmetic shift left and logical or */ \
X(SRE, self, dec)   /* Logical shift right and exclusive or */

// Addressing Modes
// X(symbol, byte count, peek template, display strings...)
#define DEC_ADDRMODE_X \
X(IMP, 1,                                       /* Implied */ \
  XPEEK(""), \
  "imp", "") \
X(IMM, 2,                                       /* Immediate */ \
  XPEEK(""), \
  "imm", "#$%02X") \
X(ZP, 2,                                        /* Zero-Page */ \
  XPEEK("= %02X", peek.data), \
  "zp", "$%02X") \
X(ZPX, 2,                                       /* Zero-Page,X */ \
  XPEEK("@ %02X = %02X", peek.finaladdr, peek.data), \
  "zp,X", "$%02X,X") \
X(ZPY, 2,                                       /* Zero-Page,Y */ \
  XPEEK("@ %02X = %02X", peek.finaladdr, peek.data), \
  "zp,Y", "$%02X,Y") \
X(INDX, 2,                                      /* (Indirect,X) */ \
  XPEEK("@ %02X > %04X = %02X", peek.interaddr, peek.finaladdr, peek.data), \
  "(zp,X)", "($%02X,X)") \
X(INDY, 2,                                      /* (Indirect),Y */ \
  XPEEK("> %04X @ %04X = %02X", peek.interaddr, peek.finaladdr, peek.data), \
  "(zp),Y", "($%02X),Y") \
X(ABS, 3,                                       /* Absolute */ \
  XPEEK("= %02X", peek.data), \
  "abs", "$??%02X", "$%04X") \
X(ABSX, 3,                                      /* Absolute,X */ \
  XPEEK("@ %04X = %02X", peek.finaladdr, peek.data), \
  "abs,X", "$??%02X,X", "$%04X,X") \
X(ABSY, 3,                                      /* Absolute,Y */ \
  XPEEK("@ %04X = %02X", peek.finaladdr, peek.data), \
  "abs,Y", "$??%02X,Y", "$%04X,Y") \
\
/* Stack */ \
X(PSH, 1,                                       /* Push */ \
  XPEEK(""), \
  "imp", "") \
X(PLL, 1,                                       /* Pull */ \
  XPEEK(""), \
  "imp", "") \
\
/* Branch */ \
X(BCH, 2,                                       /* Relative branch */ \
  XPEEK("@ %04X", peek.finaladdr), \
  "rel", "%+hhd") \
\
/* Jumps */ \
X(JSR, 3,                                       /* Jump to subroutine, */ \
  XPEEK(""), \
  "abs", "$??%02X", "$%04X") \
X(RTS, 1,                                       /* Return from subroutine */ \
  XPEEK(""), \
  "imp", "") \
X(JABS, 3,                                      /* Absolute jump */ \
  XPEEK(""), \
  "abs", "$??%02X", "$%04X") \
X(JIND, 3,                                      /* Indirect jump */ \
  XPEEK("> %04X", peek.finaladdr), \
  "(abs)", "($??%02X)", "($%04X)") \
\
/* Interrupts */ \
X(BRK, 1,                                       /* Break, interrupt, reset */ \
  XPEEK(""), \
  "imp", "%s") \
X(RTI, 1,                                       /* Return from interrupt */ \
  XPEEK(""), \
  "imp", "") \
\
/* Unofficial Modes */ \
X(JAM, 1,                                       /* Jammed instruction */ \
  XPEEK(""), \
  "imp", "")

#define IN_ENUM(s) IN_##s

enum inst {
#define X(s, ...) IN_ENUM(s),
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
    bool unofficial;
};

extern const int BrkOpcode;
extern const struct decoded Decode[];

#endif

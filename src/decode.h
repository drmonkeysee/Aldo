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
// X(symbol, description, affected flags, instruction dispatch arguments...)
#define DEC_INST_X \
X(UDF, "Undefined",                 /* Undefined */ \
  0x00, self) \
\
X(ADC, "Add with carry",            /* Add with carry */ \
  0x00, self, dec) \
X(AND, "Logical AND",               /* Logical AND */ \
  0x00, self, dec) \
X(ASL, "Arithmetic shift left",     /* Arithmetic shift left */ \
  0x00, self, dec) \
\
X(BCC, "Branch if carry clear",     /* Branch if carry clear */ \
  0x00, self) \
X(BCS, "Branch if carry set",       /* Branch if carry set */ \
  0x00, self) \
X(BEQ, "Branch if zero",            /* Branch if zero */ \
  0x00, self) \
X(BIT, "Test bits with A",          /* Test bits */ \
  0x00, self, dec) \
X(BMI, "Branch if negative ",       /* Branch if negative */ \
  0x00, self) \
X(BNE, "Branch if not zero",        /* Branch if not zero */ \
  0x00, self) \
X(BPL, "Branch if positive",        /* Branch if positive */ \
  0x00, self) \
X(BRK, "Break",                     /* Break */ \
  0x00, self) \
X(BVC, "Branch if overflow clear",  /* Branch if overflow clear */ \
  0x00, self) \
X(BVS, "Branch if overflow set",    /* Branch if overflow set */ \
  0x00, self) \
\
X(CLC, "Clear carry",               /* Clear carry */ \
  0x00, self) \
X(CLD, "Clear decimal mode",        /* Clear decimal mode */ \
  0x00, self) \
X(CLI, "Clear interrupt disable",   /* Clear interrupt disable */ \
  0x00, self) \
X(CLV, "Clear overflow",            /* Clear overflow */ \
  0x00, self) \
X(CMP, "Compare to A",              /* Compare to accumulator */ \
  0x00, self, dec) \
X(CPX, "Compare to X",              /* Compare to x-index */ \
  0x00, self, dec) \
X(CPY, "Compare to Y",              /* Compare to y-index */ \
  0x00, self, dec) \
\
X(DEC, "Decrement memory",          /* Decrement memory */ \
  0x00, self, dec) \
X(DEX, "Decrement X",               /* Decrement x-index */ \
  0x00, self) \
X(DEY, "Decrement Y",               /* Decrement y-index */ \
  0x00, self) \
\
X(EOR, "Logical EXCLUSIVE OR",      /* Logical EXCLUSIVE OR */ \
  0x00, self, dec) \
\
X(INC, "Increment memory",          /* Increment memory */ \
  0x00, self, dec) \
X(INX, "Increment X",               /* Increment x-index */ \
  0x00, self) \
X(INY, "Increment Y",               /* Increment y-index */ \
  0x00, self) \
\
X(JMP, "Jump",                      /* Jump */ \
  0x00, self) \
X(JSR, "Jump to subroutine",        /* Jump to subroutine */ \
  0x00, self) \
\
X(LDA, "Load A",                    /* Load accumulator */ \
  0x00, self, dec) \
X(LDX, "Load X",                    /* Load x-index */ \
  0x00, self, dec) \
X(LDY, "Load Y",                    /* Load y-index */ \
  0x00, self, dec) \
X(LSR, "Logical shift right",       /* Logical shift right */ \
  0x00, self, dec) \
\
X(NOP, "No operation",              /* No-op */ \
  0x00, self, dec) \
\
X(ORA, "Logical OR",                /* Logical OR */ \
  0x00, self, dec) \
\
X(PHA, "Push A",                    /* Push accumulator */ \
  0x00, self) \
X(PHP, "Push S",                    /* Push status */ \
  0x00, self) \
X(PLA, "Pull A",                    /* Pull accumulator */ \
  0x00, self) \
X(PLP, "Pull S",                    /* Pull status */ \
  0x00, self) \
\
X(ROL, "Rotate left",               /* Rotate left */ \
  0x00, self, dec) \
X(ROR, "Rotate left",               /* Rotate right */ \
  0x00, self, dec) \
X(RTI, "Return from interrupt",     /* Return from interrupt */ \
  0x00, self) \
X(RTS, "Return from subroutine",    /* Return from subroutine */ \
  0x00, self) \
\
X(SBC, "Subtract with carry",       /* Subtract with carry */ \
  0x00, self, dec) \
X(SEC, "Set carry",                 /* Set carry */ \
  0x00, self) \
X(SED, "Set decimal mode",          /* Set decimal mode */ \
  0x00, self) \
X(SEI, "Set interrupt disable",     /* Set interrupt disable */ \
  0x00, self) \
X(STA, "Store A",                   /* Store accumulator */ \
  0x00, self, dec) \
X(STX, "Store X",                   /* Store x-index */ \
  0x00, self, dec) \
X(STY, "Store Y",                   /* Store y-index */ \
  0x00, self, dec) \
\
X(TAX, "Transfer A to X",           /* Transfer accumulator to x-index */ \
  0x00, self) \
X(TAY, "Transfer A to Y",           /* Transfer accumulator to y-index */ \
  0x00, self) \
X(TSX, "Transfer SP to X",          /* Transfer stack pointer to x-index */ \
  0x00, self) \
X(TXA, "Transfer X to A",           /* Transfer x-index to accumulator */ \
  0x00, self) \
X(TXS, "Transfer X to SP",          /* Transfer x-index to stack pointer */ \
  0x00, self) \
X(TYA, "Transfer Y to A",           /* Transfer y-index to accumulator */ \
  0x00, self) \
\
/* Unofficial Opcodes */ \
X(ALR, "AND + LSR",                 /* Logical AND and logical shift right */ \
  0x00, self, dec) \
X(ANC,                              /* Logical AND and set carry as if
                                       ASL or ROL */ \
  "AND + set carry as if ASL or ROL", \
  0x00, self) \
X(ANE,                              /* Accumulator interferes with internal
                                       signals, logical AND with x-index and
                                       operand, load into accumulator */ \
  "AND with X + LDA (highly unstable)", \
  0x00, self) \
X(ARR,                              /* Logical AND and rotate right with adder
                                       side-effects interleaved between
                                       AND and ROR */ \
  "AND + ROR with adder side-effects", \
  0x00, self, dec) \
X(DCP, "DEC + CMP",                 /* Decrement memory and compare
                                       to accumulator */ \
  0x00, self, dec) \
X(ISC, "INC + SBC",                 /* Increment memory and subtract
                                       with carry */ \
  0x00, self, dec) \
X(JAM, "Jams the CPU",              /* No instruction signal, jams the cpu */ \
  0x00, self) \
X(LAS,                              /* Logical AND with stack pointer and load
                                       into accumulator, x-index, and
                                       stack pointer */ \
  "AND with SP + store SP + LDA/TSX", \
  0x00, self, dec) \
X(LAX, "LDA + LDX",                 /* Load accumulator and x-index */ \
  0x00, self, dec) \
X(LXA,                              /* Accumulator interferes with internal
                                       signals, logical AND with operand,
                                       load into accumulator and x-index */ \
  "AND + LDA/LDX (highly unstable)", \
  0x00, self) \
X(RLA, "ROL + AND",                 /* Rotate left and logical AND */ \
  0x00, self, dec) \
X(RRA, "ROR + ADC",                 /* Rotate right and add with carry */ \
  0x00, self, dec) \
X(SAX,                              /* Store logical AND of accumulator
                                       and x-index */ \
  "Store AND(A,X)", \
  0x00, self) \
X(SBX,                              /* Logical AND accumulator and x-index,
                                       compare to operand, and load into
                                       x-index */ \
  "AND(A,X) + CMP + LDX", \
  0x00, self) \
X(SHA,                              /* Store logical AND accumulator and
                                       x-index and ADDR_HI + 1 */ \
  "Store AND(A,X,ADDR_HI + 1) (unstable)", \
  0x00, self, dec) \
X(SHX,                              /* Store logical AND x-index and
                                       ADDR_HI + 1 */ \
  "Store AND(X,ADDR_HI + 1) (unstable)", \
  0x00, self, dec) \
X(SHY,                              /* Store logical AND y-index and
                                       ADDR_HI + 1 */ \
  "Store AND(Y,ADDR_HI + 1) (unstable)", \
  0x00, self, dec) \
X(SLO, "ASL + ORA",                 /* Arithmetic shift left and logical OR */ \
  0x00,self, dec) \
X(SRE, "LSR + EOR",                 /* Logical shift right and logical
                                       EXCLUSIVE OR */ \
  0x00, self, dec) \
X(TAS,                              /* Load logical AND accumulator and x-index
                                       into stack pointer, store logical AND
                                       accumulator and x-index and
                                       ADDR_HI + 1 */ \
  "Load AND(A,X) into SP + store AND(A,X,ADDR_HI + 1) (unstable)", \
  0x00, self, dec)

// Addressing Modes
// X(symbol, byte count, name, peek template, display strings...)
#define DEC_ADDRMODE_X \
X(IMP, 1, "Implied",                            /* Implied */ \
  XPEEK(""), \
  "imp", "") \
X(IMM, 2, "Immediate",                          /* Immediate */ \
  XPEEK(""), \
  "imm", "#$%02X") \
X(ZP, 2, "Zero-Page",                           /* Zero-Page */ \
  XPEEK("= %02X", peek.data), \
  "zp", "$%02X") \
X(ZPX, 2, "Zero-Page,X",                        /* Zero-Page,X */ \
  XPEEK("@ %02X = %02X", peek.finaladdr, peek.data), \
  "zp,X", "$%02X,X") \
X(ZPY, 2, "Zero-Page,Y",                        /* Zero-Page,Y */ \
  XPEEK("@ %02X = %02X", peek.finaladdr, peek.data), \
  "zp,Y", "$%02X,Y") \
X(INDX, 2, "(Indirect,X)",                      /* (Indirect,X) */ \
  XPEEK("@ %02X > %04X = %02X", peek.interaddr, peek.finaladdr, peek.data), \
  "(zp,X)", "($%02X,X)") \
X(INDY, 2, "(Indirect),Y",                      /* (Indirect),Y */ \
  XPEEK("> %04X @ %04X = %02X", peek.interaddr, peek.finaladdr, peek.data), \
  "(zp),Y", "($%02X),Y") \
X(ABS, 3, "Absolute",                           /* Absolute */ \
  XPEEK("= %02X", peek.data), \
  "abs", "$??%02X", "$%04X") \
X(ABSX, 3, "Absolute,X",                        /* Absolute,X */ \
  XPEEK("@ %04X = %02X", peek.finaladdr, peek.data), \
  "abs,X", "$??%02X,X", "$%04X,X") \
X(ABSY, 3, "Absolute,Y",                        /* Absolute,Y */ \
  XPEEK("@ %04X = %02X", peek.finaladdr, peek.data), \
  "abs,Y", "$??%02X,Y", "$%04X,Y") \
\
/* Stack */ \
X(PSH, 1, "Implied",                            /* Push */ \
  XPEEK(""), \
  "imp", "") \
X(PLL, 1, "Implied",                            /* Pull */ \
  XPEEK(""), \
  "imp", "") \
\
/* Branch */ \
X(BCH, 2, "Relative",                           /* Relative branch */ \
  XPEEK("@ %04X", peek.finaladdr), \
  "rel", "%+hhd") \
\
/* Jumps */ \
X(JSR, 3, "Absolute",                           /* Jump to subroutine, */ \
  XPEEK(""), \
  "abs", "$??%02X", "$%04X") \
X(RTS, 1, "Implied",                            /* Return from subroutine */ \
  XPEEK(""), \
  "imp", "") \
X(JABS, 3, "Absolute",                          /* Absolute jump */ \
  XPEEK(""), \
  "abs", "$??%02X", "$%04X") \
X(JIND, 3, "Indirect",                          /* Indirect jump */ \
  XPEEK("> %04X", peek.finaladdr), \
  "(abs)", "($??%02X)", "($%04X)") \
\
/* Interrupts */ \
X(BRK, 1, "Implied",                            /* Break, interrupt, reset */ \
  XPEEK(""), \
  "imp", "%s") \
X(RTI, 1, "Implied",                            /* Return from interrupt */ \
  XPEEK(""), \
  "imp", "") \
\
/* Unofficial Modes */ \
X(JAM, 1, "Implied",                            /* Jammed instruction */ \
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
    bool unofficial;
};

extern const int BrkOpcode;
extern const struct decoded Decode[];

#endif

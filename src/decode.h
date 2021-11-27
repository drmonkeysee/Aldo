//
//  decode.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#ifndef Aldo_decode_h
#define Aldo_decode_h

// 6502 Instructions
// X(symbol, instruction dispatch arguments...)
#define DEC_INST_X \
X(UNK, self)        /* Unknown */ \
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
X(CPX, self, dec)   /* Compare to x index */ \
X(CPY, self, dec)   /* Compare to y index */ \
\
X(DEC, self, dec)   /* Decrement memory */ \
X(DEX, self)        /* Decrement x index */ \
X(DEY, self)        /* Decrement y index */ \
\
X(EOR, self, dec)   /* Logical exclusive or */ \
\
X(INC, self, dec)   /* Increment memory */ \
X(INX, self)        /* Increment x index */ \
X(INY, self)        /* Increment y index */ \
\
X(JMP, self)        /* Jump */ \
X(JSR, self)        /* Jump to subroutine */ \
\
X(LDA, self, dec)   /* Load accumulator */ \
X(LDX, self, dec)   /* Load x index */ \
X(LDY, self, dec)   /* Load y index */ \
X(LSR, self, dec)   /* Logical shift right */ \
\
X(NOP, self)        /* No-op */ \
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
X(STX, self, dec)   /* Store x index */ \
X(STY, self, dec)   /* Store y index */ \
\
X(TAX, self)        /* Transfer accumulator to x index */ \
X(TAY, self)        /* Transfer accumulator to y index */ \
X(TSX, self)        /* Transfer stack pointer to x index */ \
X(TXA, self)        /* Transfer x index to accumulator */ \
X(TXS, self)        /* Transfer x index to stack pointer */ \
X(TYA, self)        /* Transfer y index to accumulator */

// Addressing Modes
// X(symbol, byte count, display strings...)
#define DEC_ADDRMODE_X \
X(IMP, 1, "imp", "")                        /* Implied */ \
X(IMM, 2, "imm", "#$%02X")                  /* Immediate */ \
X(ZP, 2, "zp", "$%02X")                     /* Zero-Page */ \
X(ZPX, 2, "zp,X", "$%02X,X")                /* Zero-Page,X */ \
X(ZPY, 2, "zp,Y", "$%02X,Y")                /* Zero-Page,Y */ \
X(INDX, 2, "(zp,X)", "($%02X,X)")           /* (Indirect,X) */ \
X(INDY, 2, "(zp),Y", "($%02X),Y")           /* (Indirect),Y */ \
X(ABS, 3, "abs", "$??%02X", "$%04X")        /* Absolute */ \
X(ABSX, 3, "abs,X", "$??%02X,X", "$%04X,X") /* Absolute,X */ \
X(ABSY, 3, "abs,Y", "$??%02X,Y", "$%04X,Y") /* Absolute,Y */ \
\
/* Stack */ \
X(PSH, 1, "imp", "")                        /* Push */ \
X(PLL, 1, "imp", "")                        /* Pull */ \
\
/* Branch */ \
X(BCH, 2, "rel", "%+hhd")                   /* Relative branch */ \
\
/* Jumps */ \
X(JSR, 3, "abs", "$??%02X", "$%04X")        /* Jump to subroutine, */ \
X(RTS, 1, "imp", "")                        /* Return from subroutine */ \
X(JABS, 3, "abs", "$??%02X", "$%04X")       /* Absolute jump */ \
X(JIND, 3, "(abs)", "($??%02X)", "($%04X)") /* Indirect jump */ \
\
/* Interrupts */ \
X(BRK, 1, "imp", "%s")                      /* Break, interrupt, reset */ \
X(RTI, 1, "imp", "")                        /* Return from interrupt */

#define IN_ENUM(s) IN_##s

enum inst {
#define X(s, ...) IN_ENUM(s),
    DEC_INST_X
#undef X
};

#define AM_ENUM(s) AM_##s

enum addrmode {
#define X(s, b, ...) AM_ENUM(s),
    DEC_ADDRMODE_X
#undef X
};

struct decoded {
    enum inst instruction;
    enum addrmode mode;
};

extern const int BrkOpcode;
extern const struct decoded Decode[];

#endif

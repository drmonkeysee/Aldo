//
//  decode.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#ifndef Aldo_emu_decode_h
#define Aldo_emu_decode_h

#include <stddef.h>

// 6502 Instructions
// X(symbol)
#define DEC_INST_X \
X(UNK)  /* Unknown */ \
\
X(ADC)  /* Add with carry */ \
X(AND)  /* Logical and */ \
X(ASL)  /* Arithmetic shift left */ \
\
X(BCC)  /* Branch if carry clear */ \
X(BCS)  /* Branch if carry set */ \
X(BEQ)  /* Branch if zero */ \
X(BIT)  /* Test bits */ \
X(BMI)  /* Branch if negative */ \
X(BNE)  /* Branch if not zero */ \
X(BPL)  /* Branch if positive */ \
X(BRK)  /* Break */ \
X(BVC)  /* Branch if overflow clear */ \
X(BVS)  /* Branch if overflow set */ \
\
X(CLC)  /* Clear carry */ \
X(CLD)  /* Clear decimal */ \
X(CLI)  /* Clear interrupt disable */ \
X(CLV)  /* Clear overflow */ \
X(CMP)  /* Compare to accumulator */ \
X(CPX)  /* Compare to x index */ \
X(CPY)  /* Compare to y index */ \
\
X(DEC)  /* Decrement memory */ \
X(DEX)  /* Decrement x index */ \
X(DEY)  /* Decrement y index */ \
\
X(EOR)  /* Logical exclusive or */ \
\
X(INC)  /* Increment memory */ \
X(INX)  /* Increment x index */ \
X(INY)  /* Increment y index */ \
\
X(JMP)  /* Jump */ \
X(JSR)  /* Jump to subroutine */ \
\
X(LDA)  /* Load accumulator */ \
X(LDX)  /* Load x index */ \
X(LDY)  /* Load y index */ \
X(LSR)  /* Logical shift right */ \
\
X(NOP)  /* No-op */ \
\
X(ORA)  /* Logical or */ \
\
X(PHA)  /* Push accumulator */ \
X(PHP)  /* Push status */ \
X(PLA)  /* Pull accumulator */ \
X(PLP)  /* Pull status */ \
\
X(ROL)  /* Rotate left */ \
X(ROR)  /* Rotate right */ \
X(RTI)  /* Return from interrupt */ \
X(RTS)  /* Return from subroutine */ \
\
X(SBC)  /* Subtract with carry */ \
X(SEC)  /* Set carry */ \
X(SED)  /* Set decimal */ \
X(SEI)  /* Set interrupt disable */ \
X(STA)  /* Store accumulator */ \
X(STX)  /* Store x index */ \
X(STY)  /* Store y index */ \
\
X(TAX)  /* Transfer accumulator to x index */ \
X(TAY)  /* Transfer accumulator to y index */ \
X(TSX)  /* Transfer stack pointer to x index */ \
X(TXA)  /* Transfer x index to accumulator */ \
X(TXS)  /* Transfer x index to stack pointer */ \
X(TYA)  /* Transfer y index to accumulator */

// Addressing Modes
// X(symbol, byte count)
#define DEC_ADDRMODE_X \
X(IMP, 1)   /* Implied */ \
X(IMM, 2)   /* Immediate */ \
X(ZP, 2)    /* Zero-page */ \
X(ZPX, 2)   /* Zero-page, X */ \
X(ZPY, 2)   /* Zero-page, Y */ \
X(INDX, 2)  /* (Indirect, X) */ \
X(INDY, 2)  /* (Indirect), Y */ \
X(ABS, 3)   /* Absolute */ \
X(ABSX, 3)  /* Absolute, X */ \
X(ABSY, 3)  /* Absolute, Y */ \
\
/* Stack */ \
X(PSH, 1)   /* Push */ \
X(PLL, 1)   /* Pull */ \
\
/* Branch */ \
X(BCH, 2)   /* Relative branch */ \
\
/* Jumps */ \
X(JSR, 3)   /* Jump to subroutine, */ \
X(RTS, 1)   /* Return from subroutine */ \
X(JABS, 3)  /* Absolute jump */ \
X(JIND, 3)  /* Indirect jump */ \
\
/* Interrupts */ \
X(BRK, 1)   /* Break, interrupt, reset */ \
X(RTI, 1)   /* Return from interrupt */

enum inst {
#define X(i) IN_##i,
    DEC_INST_X
#undef X
};

enum addrmode {
#define X(i, b) AM_##i,
    DEC_ADDRMODE_X
#undef X
};

struct decoded {
    enum inst instruction;
    enum addrmode mode;
};

extern const struct decoded Decode[];
extern const size_t DecodeSize;

#endif

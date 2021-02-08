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

// Opcode cycle sequences:
// the 6502 defines several addressing modes, however the same addressing
// mode may result in different cycle timing depending on the instruction;
// this table enumerates all possible opcode cycle sequences defined by the
// combination of instruction and addressing mode.
// read (_R)              - the operation reads from memory into registers
// write (_W)             - the operation writes from registers into memory
// read-modify-write (_M) - the operation reads from memory,
//                          modifies the value, and writes it back
#define DEC_CYCLESEQ_X \
/* Implied */ \
X(IMP)      /* Implied */ \
\
/* Immediate */ \
X(IMM)      /* Immediate */ \
\
/* Zero-page */ \
X(ZP)       /* Zero-page read/write */ \
X(ZP_M)     /* Zero-page read-modify-write */ \
\
/* Zero-page, X */ \
X(ZPX)      /* Zero-page, X read/write */ \
X(ZPX_M)    /* Zero-page, X read-modify-write */ \
\
/* Zero-page, Y */ \
X(ZPY)      /* Zero-page, Y read/write */ \
\
/* Absolute */ \
X(ABS)      /* Absolute read/write */ \
X(ABS_M)    /* Absolute read-modify-write */ \
\
/* Absolute, X */ \
X(ABSX_R)   /* Absolute, X read */ \
X(ABSX_W)   /* Absolute, X write */ \
X(ABSX_M)   /* Absolute, X read-modify-write */ \
\
/* Absolute, Y */ \
X(ABSY_R)   /* Absolute, Y read */ \
X(ABSY_W)   /* Absolute, Y write */ \
\
/* (Indirect, X) */ \
X(INDX)     /* (Indirect, X) read/write */ \
\
/* (Indirect), Y */ \
X(INDY_R)   /* (Indirect), Y read */ \
X(INDY_W)   /* (Indirect), Y write */ \
\
/* Stack */ \
X(PSH)      /* Push */ \
X(PLL)      /* Pull */ \
\
/* Branch */ \
X(BCH)      /* Relative branch */ \
\
/* Jumps */ \
X(JSR)      /* Jump to subroutine, */ \
X(RTS)      /* Return from subroutine */ \
X(JABS)     /* Absolute jump */ \
X(JIND)     /* Indirect jump */ \
\
/* Interrupts */ \
X(BRK)      /* Break, interrupt, reset */ \
X(RTI)      /* Return from interrupt */

enum inst {
#define X(i) IN_ ## i,
    DEC_INST_X
#undef X
};

enum cycle_seq {
#define X(i) CS_ ## i,
    DEC_CYCLESEQ_X
#undef X
};

struct decoded {
    enum inst instruction;
    enum cycle_seq sequence;
};

extern const struct decoded Decode[];
extern const size_t DecodeSize;

#endif

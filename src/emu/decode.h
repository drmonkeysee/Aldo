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
enum instruction {
    IN_UNK, // Unknown

    IN_ADC, // Add with carry
    IN_AND, // Logical and
    IN_ASL, // Arithmetic left shift

    IN_BCC, // Branch if carry clear
    IN_BCS, // Branch if carry set
    IN_BEQ, // Branch if zero
    IN_BIT, // Test bits
    IN_BMI, // Branch if negative
    IN_BNE, // Branch if not zero
    IN_BPL, // Branch if positive
    IN_BRK, // Break
    IN_BVC, // Branch if overflow clear
    IN_BVS, // Branch if overflow set

    IN_CLC, // Clear carry
    IN_CLD, // Clear decimal
    IN_CLI, // Clear interrupt disable
    IN_CLV, // Clear overflow
    IN_CMP, // Compare to accumulator
    IN_CPX, // Compare to x index
    IN_CPY, // Compare to y index

    IN_DEC, // Decrement memory
    IN_DEX, // Decrement x index
    IN_DEY, // Decrement y index

    IN_EOR, // Logical exclusive or

    IN_INC, // Increment memory
    IN_INX, // Increment x index
    IN_INY, // Increment y index

    IN_JMP, // Jump
    IN_JSR, // Jump to subroutine

    IN_LDA, // Load accumulator
    IN_LDX, // Load x index
    IN_LDY, // Load y index
    IN_LSR, // Logical shift right

    IN_NOP, // No-op

    IN_ORA, // Logical or

    IN_PHA, // Push accumulator
    IN_PHP, // Push status
    IN_PLA, // Pull accumulator
    IN_PLP, // Pull status

    IN_ROL, // Rotate left
    IN_ROR, // Rotate right
    IN_RTI, // Return from interrupt
    IN_RTS, // Return from subroutine

    IN_SBC, // Subtract with carry
    IN_SEC, // Set carry
    IN_SED, // Set decimal
    IN_SEI, // Set interrupt disable
    IN_STA, // Store accumulator
    IN_STX, // Store x index
    IN_STY, // Store y index

    IN_TAX, // Transfer accumulator to x index
    IN_TAY, // Transfer accumulator to y index
    IN_TSX, // Transfer stack pointer to x index
    IN_TXA, // Transfer x index to accumulator
    IN_TXS, // Transfer x index to stack pointer
    IN_TYA, // Transfer y index to accumulator
};

// Addressing mode cycle sequences;
// the 6502 defines several addressing modes, however the same addressing
// mode may result in different cycle timing depending on the instruction;
// this table defines the intersection of instruction and addressing mode to
// define all possible opcode cycle sequences, grouped by addressing
// mode.
// read (_R)              - the operation reads from memory into the cpu
// write (_W)             - the operation writes from the cpu into memory
// read-modify-write (_M) - the operation reads from memory,
//                          modifies the value, and writes it back
enum cycle_seq {
    // Implied
    AS_IMP,     // Implied

    // Immediate
    AS_IMM,     // Immediate

    // Zero-page
    AS_ZP,      // Zero-page read/write
    AS_ZP_M,    // Zero-page read-modify-write

    // Zero-page, X
    AS_ZPX,     // Zero-page, X read/write
    AS_ZPX_M,   // Zero-page, X read-modify-write

    // Zero-page, Y
    AS_ZPY,     // Zero-page, Y read/write

    // Absolute
    AS_ABS,     // Absolute read/write
    AS_ABS_M,   // Absolute read-modify-write

    // Absolute, X
    AS_ABSX_R,  // Absolute, X read
    AS_ABSX_W,  // Absolute, X write
    AS_ABSX_M,  // Absolute, X read-modify-write

    // Absolute, Y
    AS_ABSY_R,  // Absolute, Y read
    AS_ABSY_W,  // Absolute, Y write

    // (Indirect, X)
    AS_INDX,    // (Indirect, X) read/write

    // (Indirect), Y
    AS_INDY_R,  // (Indirect), Y read
    AS_INDY_W,  // (Indirect), Y write

    // Stack
    AS_PSH,     // Push
    AS_PLL,     // Pull

    // Branch
    AS_BCH,     // Relative branch

    // Jumps
    AS_JSR,     // Jump to subroutine,
    AS_RTS,     // Return from subroutine
    AS_JABS,    // Absolute jump
    AS_JIND,    // Indirect jump

    // Interrupts
    AS_BRK,     // Break, interrupt, reset
    AS_RTI,     // Return from interrupt
};

struct decoded {
    enum instruction instruction;
    enum cycle_seq sequence;
};

extern const struct decoded Decode[];
extern const size_t DecodeSize;

#endif

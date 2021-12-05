//
//  decode.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#include "decode.h"

#define UDF {IN_UDF, AM_IMP}

const int BrkOpcode = 0x0;

// Decoding table for all official MOS6502
// and unofficial Ricoh 2A03 opcodes.
const struct decoded Decode[] = {
    {IN_BRK, AM_BRK},   // 00 - BRK
    {IN_ORA, AM_INDX},  // 01 - ORA (zp,X)
    UDF,                // 02 - Undefined
    UDF,                // 03 - Undefined
    UDF,                // 04 - Undefined
    {IN_ORA, AM_ZP},    // 05 - ORA zp
    {IN_ASL, AM_ZP},    // 06 - ASL zp
    UDF,                // 07 - Undefined
    {IN_PHP, AM_PSH},   // 08 - PHP
    {IN_ORA, AM_IMM},   // 09 - ORA imm
    {IN_ASL, AM_IMP},   // 0A - ASL imp
    UDF,                // 0B - Undefined
    UDF,                // 0C - Undefined
    {IN_ORA, AM_ABS},   // 0D - ORA abs
    {IN_ASL, AM_ABS},   // 0E - ASL abs
    UDF,                // 0F - Undefined
    {IN_BPL, AM_BCH},   // 10 - BPL
    {IN_ORA, AM_INDY},  // 11 - ORA (zp),Y
    UDF,                // 12 - Undefined
    UDF,                // 13 - Undefined
    UDF,                // 14 - Undefined
    {IN_ORA, AM_ZPX},   // 15 - ORA zp,X
    {IN_ASL, AM_ZPX},   // 16 - ASL zp,X
    UDF,                // 17 - Undefined
    {IN_CLC, AM_IMP},   // 18 - CLC
    {IN_ORA, AM_ABSY},  // 19 - ORA abs,Y
    UDF,                // 1A - Undefined
    UDF,                // 1B - Undefined
    UDF,                // 1C - Undefined
    {IN_ORA, AM_ABSX},  // 1D - ORA abs,X
    {IN_ASL, AM_ABSX},  // 1E - ASL abs,X
    UDF,                // 1F - Undefined
    {IN_JSR, AM_JSR},   // 20 - JSR
    {IN_AND, AM_INDX},  // 21 - AND (zp,X)
    UDF,                // 22 - Undefined
    UDF,                // 23 - Undefined
    {IN_BIT, AM_ZP},    // 24 - BIT zp
    {IN_AND, AM_ZP},    // 25 - AND zp
    {IN_ROL, AM_ZP},    // 26 - ROL zp
    UDF,                // 27 - Undefined
    {IN_PLP, AM_PLL},   // 28 - PLP
    {IN_AND, AM_IMM},   // 29 - AND imm
    {IN_ROL, AM_IMP},   // 2A - ROL imp
    UDF,                // 2B - Undefined
    {IN_BIT, AM_ABS},   // 2C - BIT abs
    {IN_AND, AM_ABS},   // 2D - AND abs
    {IN_ROL, AM_ABS},   // 2E - ROL abs
    UDF,                // 2F - Undefined
    {IN_BMI, AM_BCH},   // 30 - BMI
    {IN_AND, AM_INDY},  // 31 - AND (zp),Y
    UDF,                // 32 - Undefined
    UDF,                // 33 - Undefined
    UDF,                // 34 - Undefined
    {IN_AND, AM_ZPX},   // 35 - AND zp,X
    {IN_ROL, AM_ZPX},   // 36 - ROL zp,X
    UDF,                // 37 - Undefined
    {IN_SEC, AM_IMP},   // 38 - SEC
    {IN_AND, AM_ABSY},  // 39 - AND abs,Y
    UDF,                // 3A - Undefined
    UDF,                // 3B - Undefined
    UDF,                // 3C - Undefined
    {IN_AND, AM_ABSX},  // 3D - AND abs,X
    {IN_ROL, AM_ABSX},  // 3E - ROL abs,X
    UDF,                // 3F - Undefined
    {IN_RTI, AM_RTI},   // 40 - RTI
    {IN_EOR, AM_INDX},  // 41 - EOR (zp,X)
    UDF,                // 42 - Undefined
    UDF,                // 43 - Undefined
    UDF,                // 44 - Undefined
    {IN_EOR, AM_ZP},    // 45 - EOR zp
    {IN_LSR, AM_ZP},    // 46 - LSR zp
    UDF,                // 47 - Undefined
    {IN_PHA, AM_PSH},   // 48 - PHA
    {IN_EOR, AM_IMM},   // 49 - EOR imm
    {IN_LSR, AM_IMP},   // 4A - LSR imp
    UDF,                // 4B - Undefined
    {IN_JMP, AM_JABS},  // 4C - JMP abs
    {IN_EOR, AM_ABS},   // 4D - EOR abs
    {IN_LSR, AM_ABS},   // 4E - LSR abs
    UDF,                // 4F - Undefined
    {IN_BVC, AM_BCH},   // 50 - BVC
    {IN_EOR, AM_INDY},  // 51 - EOR (zp),Y
    UDF,                // 52 - Undefined
    UDF,                // 53 - Undefined
    UDF,                // 54 - Undefined
    {IN_EOR, AM_ZPX},   // 55 - EOR zp,X
    {IN_LSR, AM_ZPX},   // 56 - LSR zp,X
    UDF,                // 57 - Undefined
    {IN_CLI, AM_IMP},   // 58 - CLI
    {IN_EOR, AM_ABSY},  // 59 - EOR abs,Y
    UDF,                // 5A - Undefined
    UDF,                // 5B - Undefined
    UDF,                // 5C - Undefined
    {IN_EOR, AM_ABSX},  // 5D - EOR abs,X
    {IN_LSR, AM_ABSX},  // 5E - LSR abs,X
    UDF,                // 5F - Undefined
    {IN_RTS, AM_RTS},   // 60 - RTS
    {IN_ADC, AM_INDX},  // 61 - ADC (zp,X)
    UDF,                // 62 - Undefined
    UDF,                // 63 - Undefined
    UDF,                // 64 - Undefined
    {IN_ADC, AM_ZP},    // 65 - ADC zp
    {IN_ROR, AM_ZP},    // 66 - ROR zp
    UDF,                // 67 - Undefined
    {IN_PLA, AM_PLL},   // 68 - PLA
    {IN_ADC, AM_IMM},   // 69 - ADC imm
    {IN_ROR, AM_IMP},   // 6A - ROR imp
    UDF,                // 6B - Undefined
    {IN_JMP, AM_JIND},  // 6C - JMP (abs)
    {IN_ADC, AM_ABS},   // 6D - ADC abs
    {IN_ROR, AM_ABS},   // 6E - ROR abs
    UDF,                // 6F - Undefined
    {IN_BVS, AM_BCH},   // 70 - BVS
    {IN_ADC, AM_INDY},  // 71 - ADC (zp),Y
    UDF,                // 72 - Undefined
    UDF,                // 73 - Undefined
    UDF,                // 74 - Undefined
    {IN_ADC, AM_ZPX},   // 75 - ADC zp,X
    {IN_ROR, AM_ZPX},   // 76 - ROR zp,X
    UDF,                // 77 - Undefined
    {IN_SEI, AM_IMP},   // 78 - SEI
    {IN_ADC, AM_ABSY},  // 79 - ADC abs,Y
    UDF,                // 7A - Undefined
    UDF,                // 7B - Undefined
    UDF,                // 7C - Undefined
    {IN_ADC, AM_ABSX},  // 7D - ADC abs,X
    {IN_ROR, AM_ABSX},  // 7E - ROR abs,X
    UDF,                // 7F - Undefined
    UDF,                // 80 - Undefined
    {IN_STA, AM_INDX},  // 81 - STA (zp,X)
    UDF,                // 82 - Undefined
    UDF,                // 83 - Undefined
    {IN_STY, AM_ZP},    // 84 - STY zp
    {IN_STA, AM_ZP},    // 85 - STA zp
    {IN_STX, AM_ZP},    // 86 - STX zp
    UDF,                // 87 - Undefined
    {IN_DEY, AM_IMP},   // 88 - DEY
    UDF,                // 89 - Undefined
    {IN_TXA, AM_IMP},   // 8A - TXA
    UDF,                // 8B - Undefined
    {IN_STY, AM_ABS},   // 8C - STY abs
    {IN_STA, AM_ABS},   // 8D - STA abs
    {IN_STX, AM_ABS},   // 8E - STX abs
    UDF,                // 8F - Undefined
    {IN_BCC, AM_BCH},   // 90 - BCC
    {IN_STA, AM_INDY},  // 91 - STA (zp),Y
    UDF,                // 92 - Undefined
    UDF,                // 93 - Undefined
    {IN_STY, AM_ZPX},   // 94 - STY zp,X
    {IN_STA, AM_ZPX},   // 95 - STA zp,X
    {IN_STX, AM_ZPY},   // 96 - STX zp,Y
    UDF,                // 97 - Undefined
    {IN_TYA, AM_IMP},   // 98 - TYA
    {IN_STA, AM_ABSY},  // 99 - STA abs,Y
    {IN_TXS, AM_IMP},   // 9A - TXS
    UDF,                // 9B - Undefined
    UDF,                // 9C - Undefined
    {IN_STA, AM_ABSX},  // 9D - STA abs,X
    UDF,                // 9E - Undefined
    UDF,                // 9F - Undefined
    {IN_LDY, AM_IMM},   // A0 - LDY imm
    {IN_LDA, AM_INDX},  // A1 - LDA (zp,X)
    {IN_LDX, AM_IMM},   // A2 - LDX imm
    UDF,                // A3 - Undefined
    {IN_LDY, AM_ZP},    // A4 - LDY zp
    {IN_LDA, AM_ZP},    // A5 - LDA zp
    {IN_LDX, AM_ZP},    // A6 - LDX zp
    UDF,                // A7 - Undefined
    {IN_TAY, AM_IMP},   // A8 - TAY
    {IN_LDA, AM_IMM},   // A9 - LDA imm
    {IN_TAX, AM_IMP},   // AA - TAX
    UDF,                // AB - Undefined
    {IN_LDY, AM_ABS},   // AC - LDY abs
    {IN_LDA, AM_ABS},   // AD - LDA abs
    {IN_LDX, AM_ABS},   // AE - LDX abs
    UDF,                // AF - Undefined
    {IN_BCS, AM_BCH},   // B0 - BCS
    {IN_LDA, AM_INDY},  // B1 - LDA (zp),Y
    UDF,                // B2 - Undefined
    UDF,                // B3 - Undefined
    {IN_LDY, AM_ZPX},   // B4 - LDY zp,X
    {IN_LDA, AM_ZPX},   // B5 - LDA zp,X
    {IN_LDX, AM_ZPY},   // B6 - LDX zp,Y
    UDF,                // B7 - Undefined
    {IN_CLV, AM_IMP},   // B8 - CLV
    {IN_LDA, AM_ABSY},  // B9 - LDA abs,Y
    {IN_TSX, AM_IMP},   // BA - TSX
    UDF,                // BB - Undefined
    {IN_LDY, AM_ABSX},  // BC - LDY abs,X
    {IN_LDA, AM_ABSX},  // BD - LDA abs,X
    {IN_LDX, AM_ABSY},  // BE - LDX abs,Y
    UDF,                // BF - Undefined
    {IN_CPY, AM_IMM},   // C0 - CPY imm
    {IN_CMP, AM_INDX},  // C1 - CMP (zp,X)
    UDF,                // C2 - Undefined
    UDF,                // C3 - Undefined
    {IN_CPY, AM_ZP},    // C4 - CPY zp
    {IN_CMP, AM_ZP},    // C5 - CMP zp
    {IN_DEC, AM_ZP},    // C6 - DEC zp
    UDF,                // C7 - Undefined
    {IN_INY, AM_IMP},   // C8 - INY
    {IN_CMP, AM_IMM},   // C9 - CMP imm
    {IN_DEX, AM_IMP},   // CA - DEX
    UDF,                // CB - Undefined
    {IN_CPY, AM_ABS},   // CC - CPY abs
    {IN_CMP, AM_ABS},   // CD - CMP abs
    {IN_DEC, AM_ABS},   // CE - DEC abs
    UDF,                // CF - Undefined
    {IN_BNE, AM_BCH},   // D0 - BNE
    {IN_CMP, AM_INDY},  // D1 - CMP (zp),Y
    UDF,                // D2 - Undefined
    UDF,                // D3 - Undefined
    UDF,                // D4 - Undefined
    {IN_CMP, AM_ZPX},   // D5 - CMP zp,X
    {IN_DEC, AM_ZPX},   // D6 - DEC zp,X
    UDF,                // D7 - Undefined
    {IN_CLD, AM_IMP},   // D8 - CLD
    {IN_CMP, AM_ABSY},  // D9 - CMP abs,Y
    UDF,                // DA - Undefined
    UDF,                // DB - Undefined
    UDF,                // DC - Undefined
    {IN_CMP, AM_ABSX},  // DD - CMP abs,X
    {IN_DEC, AM_ABSX},  // DE - DEC abs,X
    UDF,                // DF - Undefined
    {IN_CPX, AM_IMM},   // E0 - CPX imm
    {IN_SBC, AM_INDX},  // E1 - SBC (zp,X)
    UDF,                // E2 - Undefined
    UDF,                // E3 - Undefined
    {IN_CPX, AM_ZP},    // E4 - CPX zp
    {IN_SBC, AM_ZP},    // E5 - SBC zp
    {IN_INC, AM_ZP},    // E6 - INC zp
    UDF,                // E7 - Undefined
    {IN_INX, AM_IMP},   // E8 - INX
    {IN_SBC, AM_IMM},   // E9 - SBC imm
    {IN_NOP, AM_IMP},   // EA - NOP
    UDF,                // EB - Undefined
    {IN_CPX, AM_ABS},   // EC - CPX abs
    {IN_SBC, AM_ABS},   // ED - SBC abs
    {IN_INC, AM_ABS},   // EE - INC abs
    UDF,                // EF - Undefined
    {IN_BEQ, AM_BCH},   // F0 - BEQ
    {IN_SBC, AM_INDY},  // F1 - SBC (zp),Y
    UDF,                // F2 - Undefined
    UDF,                // F3 - Undefined
    UDF,                // F4 - Undefined
    {IN_SBC, AM_ZPX},   // F5 - SBC zp,X
    {IN_INC, AM_ZPX},   // F6 - INC zp,X
    UDF,                // F7 - Undefined
    {IN_SED, AM_IMP},   // F8 - SED
    {IN_SBC, AM_ABSY},  // F9 - SBC abs,Y
    UDF,                // FA - Undefined
    UDF,                // FB - Undefined
    UDF,                // FC - Undefined
    {IN_SBC, AM_ABSX},  // FD - SBC abs,X
    {IN_INC, AM_ABSX},  // FE - INC abs,X
    UDF,                // FF - Undefined
};

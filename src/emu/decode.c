//
//  decode.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#include "decode.h"

#define UNK {IN_UNK, AM_IMP}

// TODO: fill this out
// Decoding table for all official MOS6502
// and unofficial Ricoh 2A03 opcodes.
const struct decoded Decode[] = {
    UNK,    // 00 - BRK
    {IN_ORA, AM_INDX},    // 01 - ORA (zp,X)
    UNK,    // 02 - Unofficial
    UNK,    // 03 - Unofficial
    UNK,    // 04 - Unofficial
    {IN_ORA, AM_ZP},    // 05 - ORA zp
    {IN_ASL, AM_ZP},    // 06 - ASL zp
    UNK,    // 07 - Unofficial
    UNK,    // 08 - PHP
    {IN_ORA, AM_IMM},    // 09 - ORA imm
    {IN_ASL, AM_IMP},    // 0A - ASL imp
    UNK,    // 0B - Unofficial
    UNK,    // 0C - Unofficial
    {IN_ORA, AM_ABS},    // 0D - ORA abs
    {IN_ASL, AM_ABS},    // 0E - ASL abs
    UNK,    // 0F - Unofficial
    UNK,    // 10 - BPL
    {IN_ORA, AM_INDY},    // 11 - ORA (zp),Y
    UNK,    // 12 - Unofficial
    UNK,    // 13 - Unofficial
    UNK,    // 14 - Unofficial
    {IN_ORA, AM_ZPX},    // 15 - ORA zp,X
    {IN_ASL, AM_ZPX},    // 16 - ASL zp,X
    UNK,    // 17 - Unofficial
    {IN_CLC, AM_IMP},    // 18 - CLC
    {IN_ORA, AM_ABSY},    // 19 - ORA abs,Y
    UNK,    // 1A - Unofficial
    UNK,    // 1B - Unofficial
    UNK,    // 1C - Unofficial
    {IN_ORA, AM_ABSX},    // 1D - ORA abs,X
    {IN_ASL, AM_ABSX},    // 1E - ASL abs,X
    UNK,    // 1F - Unofficial
    UNK,    // 20 - JSR
    {IN_AND, AM_INDX},    // 21 - AND (zp,X)
    UNK,    // 22 - Unofficial
    UNK,    // 23 - Unofficial
    {IN_BIT, AM_ZP},    // 24 - BIT zp
    {IN_AND, AM_ZP},    // 25 - AND zp
    UNK,    // 26 - ROL zp
    UNK,    // 27 - Unofficial
    UNK,    // 28 - PLP
    {IN_AND, AM_IMM},    // 29 - AND imm
    {IN_ROL, AM_IMP},    // 2A - ROL imp
    UNK,    // 2B - Unofficial
    {IN_BIT, AM_ABS},    // 2C - BIT abs
    {IN_AND, AM_ABS},    // 2D - AND abs
    UNK,    // 2E - ROL abs
    UNK,    // 2F - Unofficial
    UNK,    // 30 - BMI
    {IN_AND, AM_INDY},    // 31 - AND (zp),Y
    UNK,    // 32 - Unofficial
    UNK,    // 33 - Unofficial
    UNK,    // 34 - Unofficial
    {IN_AND, AM_ZPX},    // 35 - AND zp,X
    UNK,    // 36 - ROL zp,X
    UNK,    // 37 - Unofficial
    {IN_SEC, AM_IMP},    // 38 - SEC
    {IN_AND, AM_ABSY},    // 39 - AND abs,Y
    UNK,    // 3A - Unofficial
    UNK,    // 3B - Unofficial
    UNK,    // 3C - Unofficial
    {IN_AND, AM_ABSX},    // 3D - AND abs,X
    UNK,    // 3E - ROL abs,X
    UNK,    // 3F - Unofficial
    UNK,    // 40 - RTI
    {IN_EOR, AM_INDX},    // 41 - EOR (zp,X)
    UNK,    // 42 - Unofficial
    UNK,    // 43 - Unofficial
    UNK,    // 44 - Unofficial
    {IN_EOR, AM_ZP},    // 45 - EOR zp
    {IN_LSR, AM_ZP},    // 46 - LSR zp
    UNK,    // 47 - Unofficial
    UNK,    // 48 - PHA
    {IN_EOR, AM_IMM},    // 49 - EOR imm
    {IN_LSR, AM_IMP},    // 4A - LSR imp
    UNK,    // 4B - Unofficial
    UNK,    // 4C - JMP abs
    {IN_EOR, AM_ABS},    // 4D - EOR abs
    {IN_LSR, AM_ABS},    // 4E - LSR abs
    UNK,    // 4F - Unofficial
    UNK,    // 50 - BVC
    {IN_EOR, AM_INDY},    // 51 - EOR (zp),Y
    UNK,    // 52 - Unofficial
    UNK,    // 53 - Unofficial
    UNK,    // 54 - Unofficial
    {IN_EOR, AM_ZPX},    // 55 - EOR zp,X
    {IN_LSR, AM_ZPX},    // 56 - LSR zp,X
    UNK,    // 57 - Unofficial
    {IN_CLI, AM_IMP},    // 58 - CLI
    {IN_EOR, AM_ABSY},    // 59 - EOR abs,Y
    UNK,    // 5A - Unofficial
    UNK,    // 5B - Unofficial
    UNK,    // 5C - Unofficial
    {IN_EOR, AM_ABSX},    // 5D - EOR abs,X
    {IN_LSR, AM_ABSX},    // 5E - LSR abs,X
    UNK,    // 5F - Unofficial
    UNK,    // 60 - RTS
    {IN_ADC, AM_INDX},    // 61 - ADC (zp,X)
    UNK,    // 62 - Unofficial
    UNK,    // 63 - Unofficial
    UNK,    // 64 - Unofficial
    {IN_ADC, AM_ZP},    // 65 - ADC zp
    UNK,    // 66 - ROR zp
    UNK,    // 67 - Unofficial
    UNK,    // 68 - PLA
    {IN_ADC, AM_IMM},    // 69 - ADC imm
    UNK,    // 6A - ROR imp
    UNK,    // 6B - Unofficial
    UNK,    // 6C - JMP (abs)
    {IN_ADC, AM_ABS},    // 6D - ADC abs
    UNK,    // 6E - ROR abs
    UNK,    // 6F - Unofficial
    UNK,    // 70 - BVS
    {IN_ADC, AM_INDY},    // 71 - ADC (zp),Y
    UNK,    // 72 - Unofficial
    UNK,    // 73 - Unofficial
    UNK,    // 74 - Unofficial
    {IN_ADC, AM_ZPX},    // 75 - ADC zp,X
    UNK,    // 76 - ROR zp,X
    UNK,    // 77 - Unofficial
    {IN_SEI, AM_IMP},    // 78 - SEI
    {IN_ADC, AM_ABSY},    // 79 - ADC abs,Y
    UNK,    // 7A - Unofficial
    UNK,    // 7B - Unofficial
    UNK,    // 7C - Unofficial
    {IN_ADC, AM_ABSX},    // 7D - ADC abs,X
    UNK,    // 7E - ROR abs,X
    UNK,    // 7F - Unofficial
    UNK,    // 80 - Unofficial
    {IN_STA, AM_INDX},    // 81 - STA (zp,X)
    UNK,    // 82 - Unofficial
    UNK,    // 83 - Unofficial
    {IN_STY, AM_ZP},    // 84 - STY zp
    {IN_STA, AM_ZP},    // 85 - STA zp
    {IN_STX, AM_ZP},    // 86 - STX zp
    UNK,    // 87 - Unofficial
    {IN_DEY, AM_IMP},    // 88 - DEY
    UNK,    // 89 - Unofficial
    {IN_TXA, AM_IMP},    // 8A - TXA
    UNK,    // 8B - Unofficial
    {IN_STY, AM_ABS},    // 8C - STY abs
    {IN_STA, AM_ABS},    // 8D - STA abs
    {IN_STX, AM_ABS},    // 8E - STX abs
    UNK,    // 8F - Unofficial
    UNK,    // 90 - BCC
    {IN_STA, AM_INDY},    // 91 - STA (zp),Y
    UNK,    // 92 - Unofficial
    UNK,    // 93 - Unofficial
    {IN_STY, AM_ZPX},    // 94 - STY zp,X
    {IN_STA, AM_ZPX},    // 95 - STA zp,X
    {IN_STX, AM_ZPY},    // 96 - STX zp,Y
    UNK,    // 97 - Unofficial
    {IN_TYA, AM_IMP},    // 98 - TYA
    {IN_STA, AM_ABSY},    // 99 - STA abs,Y
    {IN_TXS, AM_IMP},    // 9A - TXS
    UNK,    // 9B - Unofficial
    UNK,    // 9C - Unofficial
    {IN_STA, AM_ABSX},    // 9D - STA abs,X
    UNK,    // 9E - Unofficial
    UNK,    // 9F - Unofficial
    {IN_LDY, AM_IMM},    // A0 - LDY imm
    {IN_LDA, AM_INDX},    // A1 - LDA (zp,X)
    {IN_LDX, AM_IMM},    // A2 - LDX imm
    UNK,    // A3 - Unofficial
    {IN_LDY, AM_ZP},    // A4 - LDY zp
    {IN_LDA, AM_ZP},    // A5 - LDA zp
    {IN_LDX, AM_ZP},    // A6 - LDX zp
    UNK,    // A7 - Unofficial
    {IN_TAY, AM_IMP},    // A8 - TAY
    {IN_LDA, AM_IMM},    // A9 - LDA imm
    {IN_TAX, AM_IMP},    // AA - TAX
    UNK,    // AB - Unofficial
    {IN_LDY, AM_ABS},    // AC - LDY abs
    {IN_LDA, AM_ABS},    // AD - LDA abs
    {IN_LDX, AM_ABS},    // AE - LDX abs
    UNK,    // AF - Unofficial
    UNK,    // B0 - BCS
    {IN_LDA, AM_INDY},    // B1 - LDA (zp),Y
    UNK,    // B2 - Unofficial
    UNK,    // B3 - Unofficial
    {IN_LDY, AM_ZPX},    // B4 - LDY zp,X
    {IN_LDA, AM_ZPX},    // B5 - LDA zp,X
    {IN_LDX, AM_ZPY},    // B6 - LDX zp,Y
    UNK,    // B7 - Unofficial
    {IN_CLV, AM_IMP},    // B8 - CLV
    {IN_LDA, AM_ABSY},    // B9 - LDA abs,Y
    {IN_TSX, AM_IMP},    // BA - TSX
    UNK,    // BB - Unofficial
    {IN_LDY, AM_ABSX},    // BC - LDY abs,X
    {IN_LDA, AM_ABSX},    // BD - LDA abs,X
    {IN_LDX, AM_ABSY},    // BE - LDX abs,Y
    UNK,    // BF - Unofficial
    {IN_CPY, AM_IMM},    // C0 - CPY imm
    {IN_CMP, AM_INDX},    // C1 - CMP (zp,X)
    UNK,    // C2 - Unofficial
    UNK,    // C3 - Unofficial
    {IN_CPY, AM_ZP},    // C4 - CPY zp
    {IN_CMP, AM_ZP},    // C5 - CMP zp
    {IN_DEC, AM_ZP},    // C6 - DEC zp
    UNK,    // C7 - Unofficial
    {IN_INY, AM_IMP},    // C8 - INY
    {IN_CMP, AM_IMM},    // C9 - CMP imm
    {IN_DEX, AM_IMP},    // CA - DEX
    UNK,    // CB - Unofficial
    {IN_CPY, AM_ABS},    // CC - CPY abs
    {IN_CMP, AM_ABS},    // CD - CMP abs
    {IN_DEC, AM_ABS},    // CE - DEC abs
    UNK,    // CF - Unofficial
    UNK,    // D0 - BNE
    {IN_CMP, AM_INDY},    // D1 - CMP (zp),Y
    UNK,    // D2 - Unofficial
    UNK,    // D3 - Unofficial
    UNK,    // D4 - Unofficial
    {IN_CMP, AM_ZPX},    // D5 - CMP zp,X
    {IN_DEC, AM_ZPX},    // D6 - DEC zp,X
    UNK,    // D7 - Unofficial
    {IN_CLD, AM_IMP},    // D8 - CLD
    {IN_CMP, AM_ABSY},    // D9 - CMP abs,Y
    UNK,    // DA - Unofficial
    UNK,    // DB - Unofficial
    UNK,    // DC - Unofficial
    {IN_CMP, AM_ABSX},    // DD - CMP abs,X
    {IN_DEC, AM_ABSX},    // DE - DEC abs,X
    UNK,    // DF - Unofficial
    {IN_CPX, AM_IMM},    // E0 - CPX imm
    {IN_SBC, AM_INDX},    // E1 - SBC (zp,X)
    UNK,    // E2 - Unofficial
    UNK,    // E3 - Unofficial
    {IN_CPX, AM_ZP},    // E4 - CPX zp
    {IN_SBC, AM_ZP},    // E5 - SBC zp
    {IN_INC, AM_ZP},    // E6 - INC zp
    UNK,    // E7 - Unofficial
    {IN_INX, AM_IMP},    // E8 - INX
    {IN_SBC, AM_IMM},    // E9 - SBC imm
    {IN_NOP, AM_IMP},    // EA - NOP
    UNK,    // EB - Unofficial
    {IN_CPX, AM_ABS},    // EC - CPX abs
    {IN_SBC, AM_ABS},    // ED - SBC abs
    {IN_INC, AM_ABS},    // EE - INC abs
    UNK,    // EF - Unofficial
    UNK,    // F0 - BEQ
    {IN_SBC, AM_INDY},    // F1 - SBC (zp),Y
    UNK,    // F2 - Unofficial
    UNK,    // F3 - Unofficial
    UNK,    // F4 - Unofficial
    {IN_SBC, AM_ZPX},    // F5 - SBC zp,X
    {IN_INC, AM_ZPX},    // F6 - INC zp,X
    UNK,    // F7 - Unofficial
    {IN_SED, AM_IMP},    // F8 - SED
    {IN_SBC, AM_ABSY},    // F9 - SBC abs,Y
    UNK,    // FA - Unofficial
    UNK,    // FB - Unofficial
    UNK,    // FC - Unofficial
    {IN_SBC, AM_ABSX},    // FD - SBC abs,X
    {IN_INC, AM_ABSX},    // FE - INC abs,X
    UNK,    // FF - Unofficial
};

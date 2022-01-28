//
//  decode.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#include "decode.h"

#include <assert.h>

#define UNDEF {IN_UDF, AM_IMP, false}
#define OP(op, am) {op, am, false}
#define UP(op, am) {op, am, true}
#define JAM {IN_JAM, AM_JAM, true}

const int BrkOpcode = 0x0;

// Decoding table for all official MOS6502
// and unofficial (undocumented, illegal) Ricoh 2A03 opcodes.

// Unofficial opcodes are derived from a combination of sources:
// http://www.oxyron.de/html/opcodes02.html
// https://www.nesdev.org/6502_cpu.txt
// https://www.masswerk.at/nowgobang/2021/6502-illegal-opcodes
// NOTE: nesdev.org documents $82, $C2, and $E2 (NOP imm) will
// occasionally jam the cpu but other sources don't mention it,
// so we emulate them as stable NOPs.

const struct decoded Decode[] = {
    OP(IN_BRK, AM_BRK),     // 00 - BRK
    OP(IN_ORA, AM_INDX),    // 01 - ORA (zp,X)
    JAM,                    // 02 - *JAM (KIL, HLT)
    UP(IN_SLO, AM_INDX),    // 03 - *SLO (ASO) (zp,X)
    UP(IN_NOP, AM_ZP),      // 04 - *NOP zp
    OP(IN_ORA, AM_ZP),      // 05 - ORA zp
    OP(IN_ASL, AM_ZP),      // 06 - ASL zp
    UP(IN_SLO, AM_ZP),      // 07 - *SLO (ASO) zp
    OP(IN_PHP, AM_PSH),     // 08 - PHP
    OP(IN_ORA, AM_IMM),     // 09 - ORA imm
    OP(IN_ASL, AM_IMP),     // 0A - ASL imp
    UP(IN_ANC, AM_IMM),     // 0B - *ANC imm
    UP(IN_NOP, AM_ABS),     // 0C - *NOP abs
    OP(IN_ORA, AM_ABS),     // 0D - ORA abs
    OP(IN_ASL, AM_ABS),     // 0E - ASL abs
    UP(IN_SLO, AM_ABS),     // 0F - *SLO (ASO) abs
    OP(IN_BPL, AM_BCH),     // 10 - BPL
    OP(IN_ORA, AM_INDY),    // 11 - ORA (zp),Y
    JAM,                    // 12 - *JAM (KIL, HLT)
    UP(IN_SLO, AM_INDY),    // 13 - *SLO (ASO) (zp),Y
    UP(IN_NOP, AM_ZPX),     // 14 - *NOP zp,X
    OP(IN_ORA, AM_ZPX),     // 15 - ORA zp,X
    OP(IN_ASL, AM_ZPX),     // 16 - ASL zp,X
    UP(IN_SLO, AM_ZPX),     // 17 - *SLO (ASO) zp,X
    OP(IN_CLC, AM_IMP),     // 18 - CLC
    OP(IN_ORA, AM_ABSY),    // 19 - ORA abs,Y
    UP(IN_NOP, AM_IMP),     // 1A - *NOP
    UP(IN_SLO, AM_ABSY),    // 1B - *SLO (ASO) abs,Y
    UP(IN_NOP, AM_ABSX),    // 1C - *NOP abs,X
    OP(IN_ORA, AM_ABSX),    // 1D - ORA abs,X
    OP(IN_ASL, AM_ABSX),    // 1E - ASL abs,X
    UP(IN_SLO, AM_ABSX),    // 1F - *SLO (ASO) abs,X
    OP(IN_JSR, AM_JSR),     // 20 - JSR
    OP(IN_AND, AM_INDX),    // 21 - AND (zp,X)
    JAM,                    // 22 - *JAM (KIL, HLT)
    UP(IN_RLA, AM_INDX),    // 23 - *RLA (zp,X)
    OP(IN_BIT, AM_ZP),      // 24 - BIT zp
    OP(IN_AND, AM_ZP),      // 25 - AND zp
    OP(IN_ROL, AM_ZP),      // 26 - ROL zp
    UP(IN_RLA, AM_ZP),      // 27 - *RLA zp
    OP(IN_PLP, AM_PLL),     // 28 - PLP
    OP(IN_AND, AM_IMM),     // 29 - AND imm
    OP(IN_ROL, AM_IMP),     // 2A - ROL imp
    UP(IN_ANC, AM_IMM),     // 2B - *ANC (ANC2) imm
    OP(IN_BIT, AM_ABS),     // 2C - BIT abs
    OP(IN_AND, AM_ABS),     // 2D - AND abs
    OP(IN_ROL, AM_ABS),     // 2E - ROL abs
    UP(IN_RLA, AM_ABS),     // 2F - *RLA abs
    OP(IN_BMI, AM_BCH),     // 30 - BMI
    OP(IN_AND, AM_INDY),    // 31 - AND (zp),Y
    JAM,                    // 32 - *JAM (KIL, HLT)
    UP(IN_RLA, AM_INDY),    // 33 - *RLA (zp),Y
    UP(IN_NOP, AM_ZPX),     // 34 - *NOP zp,X
    OP(IN_AND, AM_ZPX),     // 35 - AND zp,X
    OP(IN_ROL, AM_ZPX),     // 36 - ROL zp,X
    UP(IN_RLA, AM_ZPX),     // 37 - *RLA zp,X
    OP(IN_SEC, AM_IMP),     // 38 - SEC
    OP(IN_AND, AM_ABSY),    // 39 - AND abs,Y
    UP(IN_NOP, AM_IMP),     // 3A - *NOP
    UP(IN_RLA, AM_ABSY),    // 3B - *RLA abs,Y
    UP(IN_NOP, AM_ABSX),    // 3C - *NOP abs,X
    OP(IN_AND, AM_ABSX),    // 3D - AND abs,X
    OP(IN_ROL, AM_ABSX),    // 3E - ROL abs,X
    UP(IN_RLA, AM_ABSX),    // 3F - *RLA abs,X
    OP(IN_RTI, AM_RTI),     // 40 - RTI
    OP(IN_EOR, AM_INDX),    // 41 - EOR (zp,X)
    JAM,                    // 42 - *JAM (KIL, HLT)
    UP(IN_SRE, AM_INDX),    // 43 - *SRE (LSE) (zp,X)
    UP(IN_NOP, AM_ZP),      // 44 - *NOP zp
    OP(IN_EOR, AM_ZP),      // 45 - EOR zp
    OP(IN_LSR, AM_ZP),      // 46 - LSR zp
    UP(IN_SRE, AM_ZP),      // 47 - *SRE (LSE) zp
    OP(IN_PHA, AM_PSH),     // 48 - PHA
    OP(IN_EOR, AM_IMM),     // 49 - EOR imm
    OP(IN_LSR, AM_IMP),     // 4A - LSR imp
    UP(IN_ALR, AM_IMM),     // 4B - *ALR (ASR) imm
    OP(IN_JMP, AM_JABS),    // 4C - JMP abs
    OP(IN_EOR, AM_ABS),     // 4D - EOR abs
    OP(IN_LSR, AM_ABS),     // 4E - LSR abs
    UP(IN_SRE, AM_ABS),     // 4F - *SRE (LSE) abs
    OP(IN_BVC, AM_BCH),     // 50 - BVC
    OP(IN_EOR, AM_INDY),    // 51 - EOR (zp),Y
    JAM,                    // 52 - *JAM (KIL, HLT)
    UP(IN_SRE, AM_INDY),    // 53 - *SRE (LSE) (zp),Y
    UP(IN_NOP, AM_ZPX),     // 54 - *NOP zp,X
    OP(IN_EOR, AM_ZPX),     // 55 - EOR zp,X
    OP(IN_LSR, AM_ZPX),     // 56 - LSR zp,X
    UP(IN_SRE, AM_ZPX),     // 57 - *SRE (LSE) zp,X
    OP(IN_CLI, AM_IMP),     // 58 - CLI
    OP(IN_EOR, AM_ABSY),    // 59 - EOR abs,Y
    UP(IN_NOP, AM_IMP),     // 5A - *NOP
    UP(IN_SRE, AM_ABSY),    // 5B - *SRE (LSE) abs,Y
    UP(IN_NOP, AM_ABSX),    // 5C - *NOP abs,X
    OP(IN_EOR, AM_ABSX),    // 5D - EOR abs,X
    OP(IN_LSR, AM_ABSX),    // 5E - LSR abs,X
    UP(IN_SRE, AM_ABSX),    // 5F - *SRE (LSE) abs,X
    OP(IN_RTS, AM_RTS),     // 60 - RTS
    OP(IN_ADC, AM_INDX),    // 61 - ADC (zp,X)
    JAM,                    // 62 - *JAM (KIL, HLT)
    UP(IN_RRA, AM_INDX),    // 63 - *RRA (zp,X)
    UP(IN_NOP, AM_ZP),      // 64 - *NOP zp
    OP(IN_ADC, AM_ZP),      // 65 - ADC zp
    OP(IN_ROR, AM_ZP),      // 66 - ROR zp
    UP(IN_RRA, AM_ZP),      // 67 - *RRA zp
    OP(IN_PLA, AM_PLL),     // 68 - PLA
    OP(IN_ADC, AM_IMM),     // 69 - ADC imm
    OP(IN_ROR, AM_IMP),     // 6A - ROR imp
    UNDEF,                  // 6B - Undefined
    OP(IN_JMP, AM_JIND),    // 6C - JMP (abs)
    OP(IN_ADC, AM_ABS),     // 6D - ADC abs
    OP(IN_ROR, AM_ABS),     // 6E - ROR abs
    UP(IN_RRA, AM_ABS),     // 6F - *RRA abs
    OP(IN_BVS, AM_BCH),     // 70 - BVS
    OP(IN_ADC, AM_INDY),    // 71 - ADC (zp),Y
    JAM,                    // 72 - *JAM (KIL, HLT)
    UP(IN_RRA, AM_INDY),    // 73 - *RRA (zp),Y
    UP(IN_NOP, AM_ZPX),     // 74 - *NOP zp,X
    OP(IN_ADC, AM_ZPX),     // 75 - ADC zp,X
    OP(IN_ROR, AM_ZPX),     // 76 - ROR zp,X
    UP(IN_RRA, AM_ZPX),     // 77 - *RRA zp,X
    OP(IN_SEI, AM_IMP),     // 78 - SEI
    OP(IN_ADC, AM_ABSY),    // 79 - ADC abs,Y
    UP(IN_NOP, AM_IMP),     // 7A - *NOP
    UP(IN_RRA, AM_ABSY),    // 7B - *RRA abs,Y
    UP(IN_NOP, AM_ABSX),    // 7C - *NOP abs,X
    OP(IN_ADC, AM_ABSX),    // 7D - ADC abs,X
    OP(IN_ROR, AM_ABSX),    // 7E - ROR abs,X
    UP(IN_RRA, AM_ABSX),    // 7F - *RRA abs,X
    UP(IN_NOP, AM_IMM),     // 80 - *NOP imm
    OP(IN_STA, AM_INDX),    // 81 - STA (zp,X)
    UP(IN_NOP, AM_IMM),     // 82 - *NOP imm (occasionally unstable?)
    UP(IN_SAX, AM_INDX),    // 83 - *SAX (AXS, AAX) (zp,X)
    OP(IN_STY, AM_ZP),      // 84 - STY zp
    OP(IN_STA, AM_ZP),      // 85 - STA zp
    OP(IN_STX, AM_ZP),      // 86 - STX zp
    UP(IN_SAX, AM_ZP),      // 87 - *SAX (AXS, AAX) zp
    OP(IN_DEY, AM_IMP),     // 88 - DEY
    UP(IN_NOP, AM_IMM),     // 89 - *NOP imm
    OP(IN_TXA, AM_IMP),     // 8A - TXA
    UNDEF,                  // 8B - Undefined
    OP(IN_STY, AM_ABS),     // 8C - STY abs
    OP(IN_STA, AM_ABS),     // 8D - STA abs
    OP(IN_STX, AM_ABS),     // 8E - STX abs
    UP(IN_SAX, AM_ABS),     // 8F - *SAX (AXS, AAX) abs
    OP(IN_BCC, AM_BCH),     // 90 - BCC
    OP(IN_STA, AM_INDY),    // 91 - STA (zp),Y
    JAM,                    // 92 - *JAM (KIL, HLT)
    UNDEF,                  // 93 - Undefined
    OP(IN_STY, AM_ZPX),     // 94 - STY zp,X
    OP(IN_STA, AM_ZPX),     // 95 - STA zp,X
    OP(IN_STX, AM_ZPY),     // 96 - STX zp,Y
    UP(IN_SAX, AM_ZPY),     // 97 - *SAX (AXS, AAX) zp,Y
    OP(IN_TYA, AM_IMP),     // 98 - TYA
    OP(IN_STA, AM_ABSY),    // 99 - STA abs,Y
    OP(IN_TXS, AM_IMP),     // 9A - TXS
    UNDEF,                  // 9B - Undefined
    UNDEF,                  // 9C - Undefined
    OP(IN_STA, AM_ABSX),    // 9D - STA abs,X
    UNDEF,                  // 9E - Undefined
    UNDEF,                  // 9F - Undefined
    OP(IN_LDY, AM_IMM),     // A0 - LDY imm
    OP(IN_LDA, AM_INDX),    // A1 - LDA (zp,X)
    OP(IN_LDX, AM_IMM),     // A2 - LDX imm
    UP(IN_LAX, AM_INDX),    // A3 - *LAX (zp,X)
    OP(IN_LDY, AM_ZP),      // A4 - LDY zp
    OP(IN_LDA, AM_ZP),      // A5 - LDA zp
    OP(IN_LDX, AM_ZP),      // A6 - LDX zp
    UP(IN_LAX, AM_ZP),      // A7 - *LAX zp
    OP(IN_TAY, AM_IMP),     // A8 - TAY
    OP(IN_LDA, AM_IMM),     // A9 - LDA imm
    OP(IN_TAX, AM_IMP),     // AA - TAX
    UNDEF,                  // AB - Undefined
    OP(IN_LDY, AM_ABS),     // AC - LDY abs
    OP(IN_LDA, AM_ABS),     // AD - LDA abs
    OP(IN_LDX, AM_ABS),     // AE - LDX abs
    UP(IN_LAX, AM_ABS),     // AF - *LAX abs
    OP(IN_BCS, AM_BCH),     // B0 - BCS
    OP(IN_LDA, AM_INDY),    // B1 - LDA (zp),Y
    JAM,                    // B2 - *JAM (KIL, HLT)
    UP(IN_LAX, AM_INDY),    // B3 - *LAX (zp),Y
    OP(IN_LDY, AM_ZPX),     // B4 - LDY zp,X
    OP(IN_LDA, AM_ZPX),     // B5 - LDA zp,X
    OP(IN_LDX, AM_ZPY),     // B6 - LDX zp,Y
    UP(IN_LAX, AM_ZPY),     // B7 - *LAX zp,Y
    OP(IN_CLV, AM_IMP),     // B8 - CLV
    OP(IN_LDA, AM_ABSY),    // B9 - LDA abs,Y
    OP(IN_TSX, AM_IMP),     // BA - TSX
    UNDEF,                  // BB - Undefined
    OP(IN_LDY, AM_ABSX),    // BC - LDY abs,X
    OP(IN_LDA, AM_ABSX),    // BD - LDA abs,X
    OP(IN_LDX, AM_ABSY),    // BE - LDX abs,Y
    UP(IN_LAX, AM_ABSY),    // BF - *LAX abs,Y
    OP(IN_CPY, AM_IMM),     // C0 - CPY imm
    OP(IN_CMP, AM_INDX),    // C1 - CMP (zp,X)
    UP(IN_NOP, AM_IMM),     // C2 - *NOP imm (occasionally unstable?)
    UP(IN_DCP, AM_INDX),    // C3 - *DCP (DCM) (zp,X)
    OP(IN_CPY, AM_ZP),      // C4 - CPY zp
    OP(IN_CMP, AM_ZP),      // C5 - CMP zp
    OP(IN_DEC, AM_ZP),      // C6 - DEC zp
    UP(IN_DCP, AM_ZP),      // C7 - *DCP (DCM) zp
    OP(IN_INY, AM_IMP),     // C8 - INY
    OP(IN_CMP, AM_IMM),     // C9 - CMP imm
    OP(IN_DEX, AM_IMP),     // CA - DEX
    UNDEF,                  // CB - Undefined
    OP(IN_CPY, AM_ABS),     // CC - CPY abs
    OP(IN_CMP, AM_ABS),     // CD - CMP abs
    OP(IN_DEC, AM_ABS),     // CE - DEC abs
    UP(IN_DCP, AM_ABS),     // CF - *DCP (DCM) abs
    OP(IN_BNE, AM_BCH),     // D0 - BNE
    OP(IN_CMP, AM_INDY),    // D1 - CMP (zp),Y
    JAM,                    // D2 - *JAM (KIL, HLT)
    UP(IN_DCP, AM_INDY),    // D3 - *DCP (DCM) (zp),Y
    UP(IN_NOP, AM_ZPX),     // D4 - *NOP zp,X
    OP(IN_CMP, AM_ZPX),     // D5 - CMP zp,X
    OP(IN_DEC, AM_ZPX),     // D6 - DEC zp,X
    UP(IN_DCP, AM_ZPX),     // D7 - *DCP (DCM) zp,X
    OP(IN_CLD, AM_IMP),     // D8 - CLD
    OP(IN_CMP, AM_ABSY),    // D9 - CMP abs,Y
    UP(IN_NOP, AM_IMP),     // DA - *NOP
    UP(IN_DCP, AM_ABSY),    // DB - *DCP (DCM) abs,Y
    UP(IN_NOP, AM_ABSX),    // DC - *NOP abs,X
    OP(IN_CMP, AM_ABSX),    // DD - CMP abs,X
    OP(IN_DEC, AM_ABSX),    // DE - DEC abs,X
    UP(IN_DCP, AM_ABSX),    // DF - *DCP (DCM) abs,X
    OP(IN_CPX, AM_IMM),     // E0 - CPX imm
    OP(IN_SBC, AM_INDX),    // E1 - SBC (zp,X)
    UP(IN_NOP, AM_IMM),     // E2 - *NOP imm (occasionally unstable?)
    UP(IN_ISC, AM_INDX),    // E3 - *ISC (ISB, INS) (zp,X)
    OP(IN_CPX, AM_ZP),      // E4 - CPX zp
    OP(IN_SBC, AM_ZP),      // E5 - SBC zp
    OP(IN_INC, AM_ZP),      // E6 - INC zp
    UP(IN_ISC, AM_ZP),      // E7 - *ISC (ISB, INS) zp
    OP(IN_INX, AM_IMP),     // E8 - INX
    OP(IN_SBC, AM_IMM),     // E9 - SBC imm
    OP(IN_NOP, AM_IMP),     // EA - NOP
    UP(IN_SBC, AM_IMM),     // EB - *SBC (USBC) imm
    OP(IN_CPX, AM_ABS),     // EC - CPX abs
    OP(IN_SBC, AM_ABS),     // ED - SBC abs
    OP(IN_INC, AM_ABS),     // EE - INC abs
    UP(IN_ISC, AM_ABS),     // EF - *ISC (ISB, INS) abs
    OP(IN_BEQ, AM_BCH),     // F0 - BEQ
    OP(IN_SBC, AM_INDY),    // F1 - SBC (zp),Y
    JAM,                    // F2 - *JAM (KIL, HLT)
    UP(IN_ISC, AM_INDY),    // F3 - *ISC (ISB, INS) (zp),Y
    UP(IN_NOP, AM_ZPX),     // F4 - *NOP zp,X
    OP(IN_SBC, AM_ZPX),     // F5 - SBC zp,X
    OP(IN_INC, AM_ZPX),     // F6 - INC zp,X
    UP(IN_ISC, AM_ZPX),     // F7 - *ISC (ISB, INS) zp,X
    OP(IN_SED, AM_IMP),     // F8 - SED
    OP(IN_SBC, AM_ABSY),    // F9 - SBC abs,Y
    UP(IN_NOP, AM_IMP),     // FA - *NOP
    UP(IN_ISC, AM_ABSY),    // FB - *ISC (ISB, INS) abs,Y
    UP(IN_NOP, AM_ABSX),    // FC - *NOP abs,X
    OP(IN_SBC, AM_ABSX),    // FD - SBC abs,X
    OP(IN_INC, AM_ABSX),    // FE - INC abs,X
    UP(IN_ISC, AM_ABSX),    // FF - *ISC (ISB, INS) abs,X
};

static_assert(sizeof Decode / sizeof Decode[0] == 256,
              "Incorrect decode table size!");

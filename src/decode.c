//
//  decode.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#include "decode.h"

#include <assert.h>

#define UNDEF {IN_UDF, AM_IMP, 2, false}
#define OP(op, am, c) {op, am, c, false}
#define UP(op, am, c) {op, am, c, true}
#define JAM {IN_JAM, AM_JAM, 0xff, true}

const int BrkOpcode = 0x0;

// Decoding table for all official and
// unofficial (undocumented, illegal) MOS6502 opcodes.

// Unofficial opcodes are derived from a combination of sources:
//  http://www.oxyron.de/html/opcodes02.html
//  https://www.nesdev.org/6502_cpu.txt
//  https://www.masswerk.at/nowgobang/2021/6502-illegal-opcodes
//  https://csdb.dk/release/?id=212346
// NOTE: nesdev.org states that $82, $C2, and $E2 (NOP imm) will
// occasionally jam the cpu but other sources don't mention it or
// are skeptical, so emulate them as stable NOPs.
// (?) = rumored to jam by older docs but unproven and emulated as stable
// (!) = unstable
// (!!) = extremely unstable

const struct decoded Decode[] = {
    OP(IN_BRK, AM_BRK, 0xaa),   // 00 - BRK
    OP(IN_ORA, AM_INDX, 0xaa),  // 01 - ORA (zp,X)
    JAM,                        // 02 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SLO, AM_INDX, 0xaa),  // 03 - *SLO (ASO) (zp,X)
    UP(IN_NOP, AM_ZP, 0xaa),    // 04 - *NOP (DOP, SKB, IGN) zp
    OP(IN_ORA, AM_ZP, 0xaa),    // 05 - ORA zp
    OP(IN_ASL, AM_ZP, 0xaa),    // 06 - ASL zp
    UP(IN_SLO, AM_ZP, 0xaa),    // 07 - *SLO (ASO) zp
    OP(IN_PHP, AM_PSH, 0xaa),   // 08 - PHP
    OP(IN_ORA, AM_IMM, 0xaa),   // 09 - ORA imm
    OP(IN_ASL, AM_IMP, 0xaa),   // 0A - ASL imp
    UP(IN_ANC, AM_IMM, 0xaa),   // 0B - *ANC (ANA, ANB) imm
    UP(IN_NOP, AM_ABS, 0xaa),   // 0C - *NOP (TOP, SKW, IGN) abs
    OP(IN_ORA, AM_ABS, 0xaa),   // 0D - ORA abs
    OP(IN_ASL, AM_ABS, 0xaa),   // 0E - ASL abs
    UP(IN_SLO, AM_ABS, 0xaa),   // 0F - *SLO (ASO) abs
    OP(IN_BPL, AM_BCH, 0xaa),   // 10 - BPL
    OP(IN_ORA, AM_INDY, 0xaa),  // 11 - ORA (zp, 0xaa),Y
    JAM,                        // 12 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SLO, AM_INDY, 0xaa),  // 13 - *SLO (ASO) (zp, 0xaa),Y
    UP(IN_NOP, AM_ZPX, 0xaa),   // 14 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_ORA, AM_ZPX, 0xaa),   // 15 - ORA zp,X
    OP(IN_ASL, AM_ZPX, 0xaa),   // 16 - ASL zp,X
    UP(IN_SLO, AM_ZPX, 0xaa),   // 17 - *SLO (ASO) zp,X
    OP(IN_CLC, AM_IMP, 0xaa),   // 18 - CLC
    OP(IN_ORA, AM_ABSY, 0xaa),  // 19 - ORA abs,Y
    UP(IN_NOP, AM_IMP, 0xaa),   // 1A - *NOP (NPO, UNP)
    UP(IN_SLO, AM_ABSY, 0xaa),  // 1B - *SLO (ASO) abs,Y
    UP(IN_NOP, AM_ABSX, 0xaa),  // 1C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_ORA, AM_ABSX, 0xaa),  // 1D - ORA abs,X
    OP(IN_ASL, AM_ABSX, 0xaa),  // 1E - ASL abs,X
    UP(IN_SLO, AM_ABSX, 0xaa),  // 1F - *SLO (ASO) abs,X
    OP(IN_JSR, AM_JSR, 0xaa),   // 20 - JSR
    OP(IN_AND, AM_INDX, 0xaa),  // 21 - AND (zp,X)
    JAM,                        // 22 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RLA, AM_INDX, 0xaa),  // 23 - *RLA (RLN) (zp,X)
    OP(IN_BIT, AM_ZP, 0xaa),    // 24 - BIT zp
    OP(IN_AND, AM_ZP, 0xaa),    // 25 - AND zp
    OP(IN_ROL, AM_ZP, 0xaa),    // 26 - ROL zp
    UP(IN_RLA, AM_ZP, 0xaa),    // 27 - *RLA (RLN) zp
    OP(IN_PLP, AM_PLL, 0xaa),   // 28 - PLP
    OP(IN_AND, AM_IMM, 0xaa),   // 29 - AND imm
    OP(IN_ROL, AM_IMP, 0xaa),   // 2A - ROL imp
    UP(IN_ANC, AM_IMM, 0xaa),   // 2B - *ANC (ANC2) imm
    OP(IN_BIT, AM_ABS, 0xaa),   // 2C - BIT abs
    OP(IN_AND, AM_ABS, 0xaa),   // 2D - AND abs
    OP(IN_ROL, AM_ABS, 0xaa),   // 2E - ROL abs
    UP(IN_RLA, AM_ABS, 0xaa),   // 2F - *RLA (RLN) abs
    OP(IN_BMI, AM_BCH, 0xaa),   // 30 - BMI
    OP(IN_AND, AM_INDY, 0xaa),  // 31 - AND (zp, 0xaa),Y
    JAM,                        // 32 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RLA, AM_INDY, 0xaa),  // 33 - *RLA (RLN) (zp, 0xaa),Y
    UP(IN_NOP, AM_ZPX, 0xaa),   // 34 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_AND, AM_ZPX, 0xaa),   // 35 - AND zp,X
    OP(IN_ROL, AM_ZPX, 0xaa),   // 36 - ROL zp,X
    UP(IN_RLA, AM_ZPX, 0xaa),   // 37 - *RLA (RLN) zp,X
    OP(IN_SEC, AM_IMP, 0xaa),   // 38 - SEC
    OP(IN_AND, AM_ABSY, 0xaa),  // 39 - AND abs,Y
    UP(IN_NOP, AM_IMP, 0xaa),   // 3A - *NOP (NPO, UNP)
    UP(IN_RLA, AM_ABSY, 0xaa),  // 3B - *RLA (RLN) abs,Y
    UP(IN_NOP, AM_ABSX, 0xaa),  // 3C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_AND, AM_ABSX, 0xaa),  // 3D - AND abs,X
    OP(IN_ROL, AM_ABSX, 0xaa),  // 3E - ROL abs,X
    UP(IN_RLA, AM_ABSX, 0xaa),  // 3F - *RLA (RLN) abs,X
    OP(IN_RTI, AM_RTI, 0xaa),   // 40 - RTI
    OP(IN_EOR, AM_INDX, 0xaa),  // 41 - EOR (zp,X)
    JAM,                        // 42 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SRE, AM_INDX, 0xaa),  // 43 - *SRE (LSE) (zp,X)
    UP(IN_NOP, AM_ZP, 0xaa),    // 44 - *NOP (DOP, SKB, IGN) zp
    OP(IN_EOR, AM_ZP, 0xaa),    // 45 - EOR zp
    OP(IN_LSR, AM_ZP, 0xaa),    // 46 - LSR zp
    UP(IN_SRE, AM_ZP, 0xaa),    // 47 - *SRE (LSE) zp
    OP(IN_PHA, AM_PSH, 0xaa),   // 48 - PHA
    OP(IN_EOR, AM_IMM, 0xaa),   // 49 - EOR imm
    OP(IN_LSR, AM_IMP, 0xaa),   // 4A - LSR imp
    UP(IN_ALR, AM_IMM, 0xaa),   // 4B - *ALR (ASR) imm
    OP(IN_JMP, AM_JABS, 0xaa),  // 4C - JMP abs
    OP(IN_EOR, AM_ABS, 0xaa),   // 4D - EOR abs
    OP(IN_LSR, AM_ABS, 0xaa),   // 4E - LSR abs
    UP(IN_SRE, AM_ABS, 0xaa),   // 4F - *SRE (LSE) abs
    OP(IN_BVC, AM_BCH, 0xaa),   // 50 - BVC
    OP(IN_EOR, AM_INDY, 0xaa),  // 51 - EOR (zp, 0xaa),Y
    JAM,                        // 52 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SRE, AM_INDY, 0xaa),  // 53 - *SRE (LSE) (zp, 0xaa),Y
    UP(IN_NOP, AM_ZPX, 0xaa),   // 54 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_EOR, AM_ZPX, 0xaa),   // 55 - EOR zp,X
    OP(IN_LSR, AM_ZPX, 0xaa),   // 56 - LSR zp,X
    UP(IN_SRE, AM_ZPX, 0xaa),   // 57 - *SRE (LSE) zp,X
    OP(IN_CLI, AM_IMP, 0xaa),   // 58 - CLI
    OP(IN_EOR, AM_ABSY, 0xaa),  // 59 - EOR abs,Y
    UP(IN_NOP, AM_IMP, 0xaa),   // 5A - *NOP (NPO, UNP)
    UP(IN_SRE, AM_ABSY, 0xaa),  // 5B - *SRE (LSE) abs,Y
    UP(IN_NOP, AM_ABSX, 0xaa),  // 5C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_EOR, AM_ABSX, 0xaa),  // 5D - EOR abs,X
    OP(IN_LSR, AM_ABSX, 0xaa),  // 5E - LSR abs,X
    UP(IN_SRE, AM_ABSX, 0xaa),  // 5F - *SRE (LSE) abs,X
    OP(IN_RTS, AM_RTS, 0xaa),   // 60 - RTS
    OP(IN_ADC, AM_INDX, 0xaa),  // 61 - ADC (zp,X)
    JAM,                        // 62 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RRA, AM_INDX, 0xaa),  // 63 - *RRA (RLD) (zp,X)
    UP(IN_NOP, AM_ZP, 0xaa),    // 64 - *NOP (DOP, SKB, IGN) zp
    OP(IN_ADC, AM_ZP, 0xaa),    // 65 - ADC zp
    OP(IN_ROR, AM_ZP, 0xaa),    // 66 - ROR zp
    UP(IN_RRA, AM_ZP, 0xaa),    // 67 - *RRA (RLD) zp
    OP(IN_PLA, AM_PLL, 0xaa),   // 68 - PLA
    OP(IN_ADC, AM_IMM, 0xaa),   // 69 - ADC imm
    OP(IN_ROR, AM_IMP, 0xaa),   // 6A - ROR imp
    UP(IN_ARR, AM_IMM, 0xaa),   // 6B - *ARR imm
    OP(IN_JMP, AM_JIND, 0xaa),  // 6C - JMP (abs)
    OP(IN_ADC, AM_ABS, 0xaa),   // 6D - ADC abs
    OP(IN_ROR, AM_ABS, 0xaa),   // 6E - ROR abs
    UP(IN_RRA, AM_ABS, 0xaa),   // 6F - *RRA (RLD) abs
    OP(IN_BVS, AM_BCH, 0xaa),   // 70 - BVS
    OP(IN_ADC, AM_INDY, 0xaa),  // 71 - ADC (zp, 0xaa),Y
    JAM,                        // 72 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RRA, AM_INDY, 0xaa),  // 73 - *RRA (RLD) (zp, 0xaa),Y
    UP(IN_NOP, AM_ZPX, 0xaa),   // 74 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_ADC, AM_ZPX, 0xaa),   // 75 - ADC zp,X
    OP(IN_ROR, AM_ZPX, 0xaa),   // 76 - ROR zp,X
    UP(IN_RRA, AM_ZPX, 0xaa),   // 77 - *RRA (RLD) zp,X
    OP(IN_SEI, AM_IMP, 0xaa),   // 78 - SEI
    OP(IN_ADC, AM_ABSY, 0xaa),  // 79 - ADC abs,Y
    UP(IN_NOP, AM_IMP, 0xaa),   // 7A - *NOP (NPO, UNP)
    UP(IN_RRA, AM_ABSY, 0xaa),  // 7B - *RRA (RLD) abs,Y
    UP(IN_NOP, AM_ABSX, 0xaa),  // 7C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_ADC, AM_ABSX, 0xaa),  // 7D - ADC abs,X
    OP(IN_ROR, AM_ABSX, 0xaa),  // 7E - ROR abs,X
    UP(IN_RRA, AM_ABSX, 0xaa),  // 7F - *RRA (RLD) abs,X
    UP(IN_NOP, AM_IMM, 0xaa),   // 80 - *NOP (DOP, SKB) imm
    OP(IN_STA, AM_INDX, 0xaa),  // 81 - STA (zp,X)
    UP(IN_NOP, AM_IMM, 0xaa),   // 82 - *NOP (DOP, SKB) imm (?)
    UP(IN_SAX, AM_INDX, 0xaa),  // 83 - *SAX (AXS, AAX) (zp,X)
    OP(IN_STY, AM_ZP, 0xaa),    // 84 - STY zp
    OP(IN_STA, AM_ZP, 0xaa),    // 85 - STA zp
    OP(IN_STX, AM_ZP, 0xaa),    // 86 - STX zp
    UP(IN_SAX, AM_ZP, 0xaa),    // 87 - *SAX (AXS, AAX) zp
    OP(IN_DEY, AM_IMP, 0xaa),   // 88 - DEY
    UP(IN_NOP, AM_IMM, 0xaa),   // 89 - *NOP (DOP, SKB) imm
    OP(IN_TXA, AM_IMP, 0xaa),   // 8A - TXA
    UP(IN_ANE, AM_IMM, 0xaa),   // 8B - *ANE (XAA, AXM) imm (!!)
    OP(IN_STY, AM_ABS, 0xaa),   // 8C - STY abs
    OP(IN_STA, AM_ABS, 0xaa),   // 8D - STA abs
    OP(IN_STX, AM_ABS, 0xaa),   // 8E - STX abs
    UP(IN_SAX, AM_ABS, 0xaa),   // 8F - *SAX (AXS, AAX) abs
    OP(IN_BCC, AM_BCH, 0xaa),   // 90 - BCC
    OP(IN_STA, AM_INDY, 0xaa),  // 91 - STA (zp, 0xaa),Y
    JAM,                        // 92 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SHA, AM_INDY, 0xaa),  // 93 - *SHA (AHX, AXA, TEA) (zp, 0xaa),Y (!)
    OP(IN_STY, AM_ZPX, 0xaa),   // 94 - STY zp,X
    OP(IN_STA, AM_ZPX, 0xaa),   // 95 - STA zp,X
    OP(IN_STX, AM_ZPY, 0xaa),   // 96 - STX zp,Y
    UP(IN_SAX, AM_ZPY, 0xaa),   // 97 - *SAX (AXS, AAX) zp,Y
    OP(IN_TYA, AM_IMP, 0xaa),   // 98 - TYA
    OP(IN_STA, AM_ABSY, 0xaa),  // 99 - STA abs,Y
    OP(IN_TXS, AM_IMP, 0xaa),   // 9A - TXS
    UP(IN_TAS, AM_ABSY, 0xaa),  // 9B - *TAS (XAS, SHS) abs,Y (!)
    UP(IN_SHY, AM_ABSX, 0xaa),  // 9E - *SHY (A11, SYA, SAY, TEY) abs,X (!)
    OP(IN_STA, AM_ABSX, 0xaa),  // 9D - STA abs,X
    UP(IN_SHX, AM_ABSY, 0xaa),  // 9E - *SHX (A11, SXA, XAS, TEX) abs,Y (!)
    UP(IN_SHA, AM_ABSY, 0xaa),  // 9F - *SHA (AHX, AXA, TEA) abs,Y (!)
    OP(IN_LDY, AM_IMM, 0xaa),   // A0 - LDY imm
    OP(IN_LDA, AM_INDX, 0xaa),  // A1 - LDA (zp,X)
    OP(IN_LDX, AM_IMM, 0xaa),   // A2 - LDX imm
    UP(IN_LAX, AM_INDX, 0xaa),  // A3 - *LAX (zp,X)
    OP(IN_LDY, AM_ZP, 0xaa),    // A4 - LDY zp
    OP(IN_LDA, AM_ZP, 0xaa),    // A5 - LDA zp
    OP(IN_LDX, AM_ZP, 0xaa),    // A6 - LDX zp
    UP(IN_LAX, AM_ZP, 0xaa),    // A7 - *LAX zp
    OP(IN_TAY, AM_IMP, 0xaa),   // A8 - TAY
    OP(IN_LDA, AM_IMM, 0xaa),   // A9 - LDA imm
    OP(IN_TAX, AM_IMP, 0xaa),   // AA - TAX
    UP(IN_LXA, AM_IMM, 0xaa),   // AB - *LXA (LAX, ATX, OAL, ANX) imm (!!)
    OP(IN_LDY, AM_ABS, 0xaa),   // AC - LDY abs
    OP(IN_LDA, AM_ABS, 0xaa),   // AD - LDA abs
    OP(IN_LDX, AM_ABS, 0xaa),   // AE - LDX abs
    UP(IN_LAX, AM_ABS, 0xaa),   // AF - *LAX abs
    OP(IN_BCS, AM_BCH, 0xaa),   // B0 - BCS
    OP(IN_LDA, AM_INDY, 0xaa),  // B1 - LDA (zp, 0xaa),Y
    JAM,                        // B2 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_LAX, AM_INDY, 0xaa),  // B3 - *LAX (zp, 0xaa),Y
    OP(IN_LDY, AM_ZPX, 0xaa),   // B4 - LDY zp,X
    OP(IN_LDA, AM_ZPX, 0xaa),   // B5 - LDA zp,X
    OP(IN_LDX, AM_ZPY, 0xaa),   // B6 - LDX zp,Y
    UP(IN_LAX, AM_ZPY, 0xaa),   // B7 - *LAX zp,Y
    OP(IN_CLV, AM_IMP, 0xaa),   // B8 - CLV
    OP(IN_LDA, AM_ABSY, 0xaa),  // B9 - LDA abs,Y
    OP(IN_TSX, AM_IMP, 0xaa),   // BA - TSX
    UP(IN_LAS, AM_ABSY, 0xaa),  // BB - *LAS (LAR) abs,Y
    OP(IN_LDY, AM_ABSX, 0xaa),  // BC - LDY abs,X
    OP(IN_LDA, AM_ABSX, 0xaa),  // BD - LDA abs,X
    OP(IN_LDX, AM_ABSY, 0xaa),  // BE - LDX abs,Y
    UP(IN_LAX, AM_ABSY, 0xaa),  // BF - *LAX abs,Y
    OP(IN_CPY, AM_IMM, 0xaa),   // C0 - CPY imm
    OP(IN_CMP, AM_INDX, 0xaa),  // C1 - CMP (zp,X)
    UP(IN_NOP, AM_IMM, 0xaa),   // C2 - *NOP (DOP, SKB) imm (?)
    UP(IN_DCP, AM_INDX, 0xaa),  // C3 - *DCP (DCM) (zp,X)
    OP(IN_CPY, AM_ZP, 0xaa),    // C4 - CPY zp
    OP(IN_CMP, AM_ZP, 0xaa),    // C5 - CMP zp
    OP(IN_DEC, AM_ZP, 0xaa),    // C6 - DEC zp
    UP(IN_DCP, AM_ZP, 0xaa),    // C7 - *DCP (DCM) zp
    OP(IN_INY, AM_IMP, 0xaa),   // C8 - INY
    OP(IN_CMP, AM_IMM, 0xaa),   // C9 - CMP imm
    OP(IN_DEX, AM_IMP, 0xaa),   // CA - DEX
    UP(IN_SBX, AM_IMM, 0xaa),   // CB - *SBX (AXS, SAX, XMA) imm
    OP(IN_CPY, AM_ABS, 0xaa),   // CC - CPY abs
    OP(IN_CMP, AM_ABS, 0xaa),   // CD - CMP abs
    OP(IN_DEC, AM_ABS, 0xaa),   // CE - DEC abs
    UP(IN_DCP, AM_ABS, 0xaa),   // CF - *DCP (DCM) abs
    OP(IN_BNE, AM_BCH, 0xaa),   // D0 - BNE
    OP(IN_CMP, AM_INDY, 0xaa),  // D1 - CMP (zp, 0xaa),Y
    JAM,                        // D2 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_DCP, AM_INDY, 0xaa),  // D3 - *DCP (DCM) (zp, 0xaa),Y
    UP(IN_NOP, AM_ZPX, 0xaa),   // D4 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_CMP, AM_ZPX, 0xaa),   // D5 - CMP zp,X
    OP(IN_DEC, AM_ZPX, 0xaa),   // D6 - DEC zp,X
    UP(IN_DCP, AM_ZPX, 0xaa),   // D7 - *DCP (DCM) zp,X
    OP(IN_CLD, AM_IMP, 0xaa),   // D8 - CLD
    OP(IN_CMP, AM_ABSY, 0xaa),  // D9 - CMP abs,Y
    UP(IN_NOP, AM_IMP, 0xaa),   // DA - *NOP (NPO, UNP)
    UP(IN_DCP, AM_ABSY, 0xaa),  // DB - *DCP (DCM) abs,Y
    UP(IN_NOP, AM_ABSX, 0xaa),  // DC - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_CMP, AM_ABSX, 0xaa),  // DD - CMP abs,X
    OP(IN_DEC, AM_ABSX, 0xaa),  // DE - DEC abs,X
    UP(IN_DCP, AM_ABSX, 0xaa),  // DF - *DCP (DCM) abs,X
    OP(IN_CPX, AM_IMM, 0xaa),   // E0 - CPX imm
    OP(IN_SBC, AM_INDX, 0xaa),  // E1 - SBC (zp,X)
    UP(IN_NOP, AM_IMM, 0xaa),   // E2 - *NOP (DOP, SKB) imm (?)
    UP(IN_ISC, AM_INDX, 0xaa),  // E3 - *ISC (ISB, INS) (zp,X)
    OP(IN_CPX, AM_ZP, 0xaa),    // E4 - CPX zp
    OP(IN_SBC, AM_ZP, 0xaa),    // E5 - SBC zp
    OP(IN_INC, AM_ZP, 0xaa),    // E6 - INC zp
    UP(IN_ISC, AM_ZP, 0xaa),    // E7 - *ISC (ISB, INS) zp
    OP(IN_INX, AM_IMP, 0xaa),   // E8 - INX
    OP(IN_SBC, AM_IMM, 0xaa),   // E9 - SBC imm
    OP(IN_NOP, AM_IMP, 0xaa),   // EA - NOP
    UP(IN_SBC, AM_IMM, 0xaa),   // EB - *SBC (USBC, USB) imm
    OP(IN_CPX, AM_ABS, 0xaa),   // EC - CPX abs
    OP(IN_SBC, AM_ABS, 0xaa),   // ED - SBC abs
    OP(IN_INC, AM_ABS, 0xaa),   // EE - INC abs
    UP(IN_ISC, AM_ABS, 0xaa),   // EF - *ISC (ISB, INS) abs
    OP(IN_BEQ, AM_BCH, 0xaa),   // F0 - BEQ
    OP(IN_SBC, AM_INDY, 0xaa),  // F1 - SBC (zp, 0xaa),Y
    JAM,                        // F2 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_ISC, AM_INDY, 0xaa),  // F3 - *ISC (ISB, INS) (zp, 0xaa),Y
    UP(IN_NOP, AM_ZPX, 0xaa),   // F4 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_SBC, AM_ZPX, 0xaa),   // F5 - SBC zp,X
    OP(IN_INC, AM_ZPX, 0xaa),   // F6 - INC zp,X
    UP(IN_ISC, AM_ZPX, 0xaa),   // F7 - *ISC (ISB, INS) zp,X
    OP(IN_SED, AM_IMP, 0xaa),   // F8 - SED
    OP(IN_SBC, AM_ABSY, 0xaa),  // F9 - SBC abs,Y
    UP(IN_NOP, AM_IMP, 0xaa),   // FA - *NOP (NPO, UNP)
    UP(IN_ISC, AM_ABSY, 0xaa),  // FB - *ISC (ISB, INS) abs,Y
    UP(IN_NOP, AM_ABSX, 0xaa),  // FC - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_SBC, AM_ABSX, 0xaa),  // FD - SBC abs,X
    OP(IN_INC, AM_ABSX, 0xaa),  // FE - INC abs,X
    UP(IN_ISC, AM_ABSX, 0xaa),  // FF - *ISC (ISB, INS) abs,X
};

static_assert(sizeof Decode / sizeof Decode[0] == 256,
              "Incorrect decode table size!");

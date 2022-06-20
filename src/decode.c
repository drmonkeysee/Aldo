//
//  decode.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#include "decode.h"

#include <assert.h>

#define N {0}
#define AP {.a = true, .p = true}
#define I {.pc = true}
#define P {.p = true}
#define SI {.s = true, .pc = true}
#define PM {.p = true, .m = true}

#define CY(n) {.count = n}
#define PG(n) {.count = n, .page_boundary = true}
#define BR(n) {n, true, true}

#define UNDEF {IN_UDF, AM_IMP, {0}, CY(2), false}
#define OP(op, am, d, c) {op, am, d, c, false}
#define UP(op, am, d, c) {op, am, d, c, true}
#define JAM {IN_JAM, AM_JAM, {0}, CY(-1), true}

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
    OP(IN_BRK, AM_BRK, FOO, CY(7)),  // 00 - BRK
    OP(IN_ORA, AM_INDX, AP, CY(6)), // 01 - ORA (zp,X)
    JAM,                        // 02 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SLO, AM_INDX, FOO, CY(8)), // 03 - *SLO (ASO) (zp,X)
    UP(IN_NOP, AM_ZP, FOO, CY(3)),   // 04 - *NOP (DOP, SKB, IGN) zp
    OP(IN_ORA, AM_ZP, AP, CY(3)),   // 05 - ORA zp
    OP(IN_ASL, AM_ZP, PM, CY(5)),   // 06 - ASL zp
    UP(IN_SLO, AM_ZP, FOO, CY(5)),   // 07 - *SLO (ASO) zp
    OP(IN_PHP, AM_PSH, FOO, CY(3)),  // 08 - PHP
    OP(IN_ORA, AM_IMM, AP, CY(2)),  // 09 - ORA imm
    OP(IN_ASL, AM_IMP, AP, CY(2)),  // 0A - ASL imp
    UP(IN_ANC, AM_IMM, FOO, CY(2)),  // 0B - *ANC (ANA, ANB) imm
    UP(IN_NOP, AM_ABS, FOO, CY(4)),  // 0C - *NOP (TOP, SKW, IGN) abs
    OP(IN_ORA, AM_ABS, AP, CY(4)),  // 0D - ORA abs
    OP(IN_ASL, AM_ABS, PM, CY(6)),  // 0E - ASL abs
    UP(IN_SLO, AM_ABS, FOO, CY(6)),  // 0F - *SLO (ASO) abs
    OP(IN_BPL, AM_BCH, I, BR(2)),  // 10 - BPL
    OP(IN_ORA, AM_INDY, AP, PG(5)), // 11 - ORA (zp),Y
    JAM,                        // 12 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SLO, AM_INDY, FOO, CY(8)), // 13 - *SLO (ASO) (zp),Y
    UP(IN_NOP, AM_ZPX, FOO, CY(4)),  // 14 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_ORA, AM_ZPX, AP, CY(4)),  // 15 - ORA zp,X
    OP(IN_ASL, AM_ZPX, PM, CY(6)),  // 16 - ASL zp,X
    UP(IN_SLO, AM_ZPX, FOO, CY(6)),  // 17 - *SLO (ASO) zp,X
    OP(IN_CLC, AM_IMP, P, CY(2)),  // 18 - CLC
    OP(IN_ORA, AM_ABSY, AP, PG(4)), // 19 - ORA abs,Y
    UP(IN_NOP, AM_IMP, FOO, CY(2)),  // 1A - *NOP (NPO, UNP)
    UP(IN_SLO, AM_ABSY, FOO, CY(7)), // 1B - *SLO (ASO) abs,Y
    UP(IN_NOP, AM_ABSY, FOO, PG(4)), // 1C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_ORA, AM_ABSY, AP, PG(4)), // 1D - ORA abs,X
    OP(IN_ASL, AM_ABSY, PM, CY(7)), // 1E - ASL abs,X
    UP(IN_SLO, AM_ABSY, FOO, CY(7)), // 1F - *SLO (ASO) abs,X
    OP(IN_JSR, AM_JSR, FOO, CY(6)),  // 20 - JSR
    OP(IN_AND, AM_INDX, AP, CY(6)), // 21 - AND (zp,X)
    JAM,                        // 22 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RLA, AM_INDX, FOO, CY(8)), // 23 - *RLA (RLN) (zp,X)
    OP(IN_BIT, AM_ZP, P, CY(3)),   // 24 - BIT zp
    OP(IN_AND, AM_ZP, AP, CY(3)),   // 25 - AND zp
    OP(IN_ROL, AM_ZP, PM, CY(5)),   // 26 - ROL zp
    UP(IN_RLA, AM_ZP, FOO, CY(5)),   // 27 - *RLA (RLN) zp
    OP(IN_PLP, AM_PLL, FOO, CY(4)),  // 28 - PLP
    OP(IN_AND, AM_IMM, AP, CY(2)),  // 29 - AND imm
    OP(IN_ROL, AM_IMP, AP, CY(2)),  // 2A - ROL imp
    UP(IN_ANC, AM_IMM, FOO, CY(2)),  // 2B - *ANC (ANC2) imm
    OP(IN_BIT, AM_ABS, P, CY(4)),  // 2C - BIT abs
    OP(IN_AND, AM_ABS, AP, CY(4)),  // 2D - AND abs
    OP(IN_ROL, AM_ABS, PM, CY(6)),  // 2E - ROL abs
    UP(IN_RLA, AM_ABS, FOO, CY(6)),  // 2F - *RLA (RLN) abs
    OP(IN_BMI, AM_BCH, I, BR(2)),  // 30 - BMI
    OP(IN_AND, AM_INDY, AP, PG(5)), // 31 - AND (zp),Y
    JAM,                        // 32 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RLA, AM_INDY, FOO, CY(8)), // 33 - *RLA (RLN) (zp),Y
    UP(IN_NOP, AM_ZPX, FOO, CY(4)),  // 34 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_AND, AM_ZPX, AP, CY(4)),  // 35 - AND zp,X
    OP(IN_ROL, AM_ZPX, PM, CY(6)),  // 36 - ROL zp,X
    UP(IN_RLA, AM_ZPX, FOO, CY(6)),  // 37 - *RLA (RLN) zp,X
    OP(IN_SEC, AM_IMP, P, CY(2)),  // 38 - SEC
    OP(IN_AND, AM_ABSY, AP, PG(4)), // 39 - AND abs,Y
    UP(IN_NOP, AM_IMP, FOO, CY(2)),  // 3A - *NOP (NPO, UNP)
    UP(IN_RLA, AM_ABSY, FOO, CY(7)), // 3B - *RLA (RLN) abs,Y
    UP(IN_NOP, AM_ABSY, FOO, PG(4)), // 3C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_AND, AM_ABSY, AP, PG(4)), // 3D - AND abs,X
    OP(IN_ROL, AM_ABSY, PM, CY(7)), // 3E - ROL abs,X
    UP(IN_RLA, AM_ABSY, FOO, CY(7)), // 3F - *RLA (RLN) abs,X
    OP(IN_RTI, AM_RTI, FOO, CY(6)),  // 40 - RTI
    OP(IN_EOR, AM_INDX, AP, CY(6)), // 41 - EOR (zp,X)
    JAM,                        // 42 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SRE, AM_INDX, FOO, CY(8)), // 43 - *SRE (LSE) (zp,X)
    UP(IN_NOP, AM_ZP, FOO, CY(3)),   // 44 - *NOP (DOP, SKB, IGN) zp
    OP(IN_EOR, AM_ZP, AP, CY(3)),   // 45 - EOR zp
    OP(IN_LSR, AM_ZP, PM, CY(5)),   // 46 - LSR zp
    UP(IN_SRE, AM_ZP, FOO, CY(5)),   // 47 - *SRE (LSE) zp
    OP(IN_PHA, AM_PSH, FOO, CY(3)),  // 48 - PHA
    OP(IN_EOR, AM_IMM, AP, CY(2)),  // 49 - EOR imm
    OP(IN_LSR, AM_IMP, AP, CY(2)),  // 4A - LSR imp
    UP(IN_ALR, AM_IMM, FOO, CY(2)),  // 4B - *ALR (ASR) imm
    OP(IN_JMP, AM_JABS, FOO, CY(3)), // 4C - JMP abs
    OP(IN_EOR, AM_ABS, AP, CY(4)),  // 4D - EOR abs
    OP(IN_LSR, AM_ABS, PM, CY(6)),  // 4E - LSR abs
    UP(IN_SRE, AM_ABS, FOO, CY(6)),  // 4F - *SRE (LSE) abs
    OP(IN_BVC, AM_BCH, FOO, BR(2)),  // 50 - BVC
    OP(IN_EOR, AM_INDY, AP, PG(5)), // 51 - EOR (zp),Y
    JAM,                        // 52 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SRE, AM_INDY, FOO, CY(8)), // 53 - *SRE (LSE) (zp),Y
    UP(IN_NOP, AM_ZPX, FOO, CY(4)),  // 54 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_EOR, AM_ZPX, AP, CY(4)),  // 55 - EOR zp,X
    OP(IN_LSR, AM_ZPX, PM, CY(6)),  // 56 - LSR zp,X
    UP(IN_SRE, AM_ZPX, FOO, CY(6)),  // 57 - *SRE (LSE) zp,X
    OP(IN_CLI, AM_IMP, P, CY(2)),  // 58 - CLI
    OP(IN_EOR, AM_ABSY, AP, PG(4)), // 59 - EOR abs,Y
    UP(IN_NOP, AM_IMP, FOO, CY(2)),  // 5A - *NOP (NPO, UNP)
    UP(IN_SRE, AM_ABSY, FOO, CY(7)), // 5B - *SRE (LSE) abs,Y
    UP(IN_NOP, AM_ABSY, FOO, PG(4)), // 5C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_EOR, AM_ABSY, AP, PG(4)), // 5D - EOR abs,X
    OP(IN_LSR, AM_ABSY, PM, CY(7)), // 5E - LSR abs,X
    UP(IN_SRE, AM_ABSY, FOO, CY(7)), // 5F - *SRE (LSE) abs,X
    OP(IN_RTS, AM_RTS, FOO, CY(6)),  // 60 - RTS
    OP(IN_ADC, AM_INDX, AP, CY(6)), // 61 - ADC (zp,X)
    JAM,                        // 62 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RRA, AM_INDX, FOO, CY(8)), // 63 - *RRA (RLD) (zp,X)
    UP(IN_NOP, AM_ZP, FOO, CY(3)),   // 64 - *NOP (DOP, SKB, IGN) zp
    OP(IN_ADC, AM_ZP, AP, CY(3)),   // 65 - ADC zp
    OP(IN_ROR, AM_ZP, PM, CY(5)),   // 66 - ROR zp
    UP(IN_RRA, AM_ZP, FOO, CY(5)),   // 67 - *RRA (RLD) zp
    OP(IN_PLA, AM_PLL, FOO, CY(4)),  // 68 - PLA
    OP(IN_ADC, AM_IMM, AP, CY(2)),  // 69 - ADC imm
    OP(IN_ROR, AM_IMP, AP, CY(2)),  // 6A - ROR imp
    UP(IN_ARR, AM_IMM, FOO, CY(2)),  // 6B - *ARR imm
    OP(IN_JMP, AM_JIND, FOO, CY(5)), // 6C - JMP (abs)
    OP(IN_ADC, AM_ABS, AP, CY(4)),  // 6D - ADC abs
    OP(IN_ROR, AM_ABS, PM, CY(6)),  // 6E - ROR abs
    UP(IN_RRA, AM_ABS, FOO, CY(6)),  // 6F - *RRA (RLD) abs
    OP(IN_BVS, AM_BCH, FOO, BR(2)),  // 70 - BVS
    OP(IN_ADC, AM_INDY, AP, PG(5)), // 71 - ADC (zp),Y
    JAM,                        // 72 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_RRA, AM_INDY, FOO, CY(8)), // 73 - *RRA (RLD) (zp),Y
    UP(IN_NOP, AM_ZPX, FOO, CY(4)),  // 74 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_ADC, AM_ZPX, AP, CY(4)),  // 75 - ADC zp,X
    OP(IN_ROR, AM_ZPX, PM, CY(6)),  // 76 - ROR zp,X
    UP(IN_RRA, AM_ZPX, FOO, CY(6)),  // 77 - *RRA (RLD) zp,X
    OP(IN_SEI, AM_IMP, P, CY(2)),  // 78 - SEI
    OP(IN_ADC, AM_ABSY, AP, PG(4)), // 79 - ADC abs,Y
    UP(IN_NOP, AM_IMP, FOO, CY(2)),  // 7A - *NOP (NPO, UNP)
    UP(IN_RRA, AM_ABSY, FOO, CY(7)), // 7B - *RRA (RLD) abs,Y
    UP(IN_NOP, AM_ABSY, FOO, PG(4)), // 7C - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_ADC, AM_ABSY, AP, PG(4)), // 7D - ADC abs,X
    OP(IN_ROR, AM_ABSY, PM, CY(7)), // 7E - ROR abs,X
    UP(IN_RRA, AM_ABSY, FOO, CY(7)), // 7F - *RRA (RLD) abs,X
    UP(IN_NOP, AM_IMM, FOO, CY(2)),  // 80 - *NOP (DOP, SKB) imm
    OP(IN_STA, AM_INDX, FOO, CY(6)), // 81 - STA (zp,X)
    UP(IN_NOP, AM_IMM, FOO, CY(2)),  // 82 - *NOP (DOP, SKB) imm (?)
    UP(IN_SAX, AM_INDX, FOO, CY(6)), // 83 - *SAX (AXS, AAX) (zp,X)
    OP(IN_STY, AM_ZP, FOO, CY(3)),   // 84 - STY zp
    OP(IN_STA, AM_ZP, FOO, CY(3)),   // 85 - STA zp
    OP(IN_STX, AM_ZP, FOO, CY(3)),   // 86 - STX zp
    UP(IN_SAX, AM_ZP, FOO, CY(3)),   // 87 - *SAX (AXS, AAX) zp
    OP(IN_DEY, AM_IMP, FOO, CY(2)),  // 88 - DEY
    UP(IN_NOP, AM_IMM, FOO, CY(2)),  // 89 - *NOP (DOP, SKB) imm
    OP(IN_TXA, AM_IMP, AP, CY(2)),  // 8A - TXA
    UP(IN_ANE, AM_IMM, FOO, CY(2)),  // 8B - *ANE (XAA, AXM) imm (!!)
    OP(IN_STY, AM_ABS, FOO, CY(4)),  // 8C - STY abs
    OP(IN_STA, AM_ABS, FOO, CY(4)),  // 8D - STA abs
    OP(IN_STX, AM_ABS, FOO, CY(4)),  // 8E - STX abs
    UP(IN_SAX, AM_ABS, FOO, CY(4)),  // 8F - *SAX (AXS, AAX) abs
    OP(IN_BCC, AM_BCH, I, BR(2)),  // 90 - BCC
    OP(IN_STA, AM_INDY, FOO, CY(6)), // 91 - STA (zp),Y
    JAM,                        // 92 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_SHA, AM_INDY, FOO, CY(6)), // 93 - *SHA (AHX, AXA, TEA) (zp),Y (!)
    OP(IN_STY, AM_ZPX, FOO, CY(4)),  // 94 - STY zp,X
    OP(IN_STA, AM_ZPX, FOO, CY(4)),  // 95 - STA zp,X
    OP(IN_STX, AM_ZPY, FOO, CY(4)),  // 96 - STX zp,Y
    UP(IN_SAX, AM_ZPY, FOO, CY(4)),  // 97 - *SAX (AXS, AAX) zp,Y
    OP(IN_TYA, AM_IMP, AP, CY(2)),  // 98 - TYA
    OP(IN_STA, AM_ABSY, FOO, CY(5)), // 99 - STA abs,Y
    OP(IN_TXS, AM_IMP, FOO, CY(2)),  // 9A - TXS
    UP(IN_TAS, AM_ABSY, FOO, CY(5)), // 9B - *TAS (XAS, SHS) abs,Y (!)
    UP(IN_SHY, AM_ABSY, FOO, CY(5)), // 9E - *SHY (A11, SYA, SAY, TEY) abs,X (!)
    OP(IN_STA, AM_ABSY, FOO, CY(5)), // 9D - STA abs,X
    UP(IN_SHX, AM_ABSY, FOO, CY(5)), // 9E - *SHX (A11, SXA, XAS, TEX) abs,Y (!)
    UP(IN_SHA, AM_ABSY, FOO, CY(5)), // 9F - *SHA (AHX, AXA, TEA) abs,Y (!)
    OP(IN_LDY, AM_IMM, FOO, CY(2)),  // A0 - LDY imm
    OP(IN_LDA, AM_INDX, AP, CY(6)), // A1 - LDA (zp,X)
    OP(IN_LDX, AM_IMM, FOO, CY(2)),  // A2 - LDX imm
    UP(IN_LAX, AM_INDX, FOO, CY(6)), // A3 - *LAX (zp,X)
    OP(IN_LDY, AM_ZP, FOO, CY(3)),   // A4 - LDY zp
    OP(IN_LDA, AM_ZP, AP, CY(3)),   // A5 - LDA zp
    OP(IN_LDX, AM_ZP, FOO, CY(3)),   // A6 - LDX zp
    UP(IN_LAX, AM_ZP, FOO, CY(3)),   // A7 - *LAX zp
    OP(IN_TAY, AM_IMP, FOO, CY(2)),  // A8 - TAY
    OP(IN_LDA, AM_IMM, AP, CY(2)),  // A9 - LDA imm
    OP(IN_TAX, AM_IMP, FOO, CY(2)),  // AA - TAX
    UP(IN_LXA, AM_IMM, FOO, CY(2)),  // AB - *LXA (LAX, ATX, OAL, ANX) imm (!!)
    OP(IN_LDY, AM_ABS, FOO, CY(4)),  // AC - LDY abs
    OP(IN_LDA, AM_ABS, AP, CY(4)),  // AD - LDA abs
    OP(IN_LDX, AM_ABS, FOO, CY(4)),  // AE - LDX abs
    UP(IN_LAX, AM_ABS, FOO, CY(4)),  // AF - *LAX abs
    OP(IN_BCS, AM_BCH, I, BR(2)),  // B0 - BCS
    OP(IN_LDA, AM_INDY, AP, PG(5)), // B1 - LDA (zp),Y
    JAM,                        // B2 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_LAX, AM_INDY, FOO, PG(5)), // B3 - *LAX (zp),Y
    OP(IN_LDY, AM_ZPX, FOO, CY(4)),  // B4 - LDY zp,X
    OP(IN_LDA, AM_ZPX, AP, CY(4)),  // B5 - LDA zp,X
    OP(IN_LDX, AM_ZPY, FOO, CY(4)),  // B6 - LDX zp,Y
    UP(IN_LAX, AM_ZPY, FOO, CY(4)),  // B7 - *LAX zp,Y
    OP(IN_CLV, AM_IMP, P, CY(2)),  // B8 - CLV
    OP(IN_LDA, AM_ABSY, AP, PG(4)), // B9 - LDA abs,Y
    OP(IN_TSX, AM_IMP, FOO, CY(2)),  // BA - TSX
    UP(IN_LAS, AM_ABSY, FOO, PG(4)), // BB - *LAS (LAR) abs,Y
    OP(IN_LDY, AM_ABSY, FOO, PG(4)), // BC - LDY abs,X
    OP(IN_LDA, AM_ABSY, AP, PG(4)), // BD - LDA abs,X
    OP(IN_LDX, AM_ABSY, FOO, PG(4)), // BE - LDX abs,Y
    UP(IN_LAX, AM_ABSY, FOO, PG(4)), // BF - *LAX abs,Y
    OP(IN_CPY, AM_IMM, P, CY(2)),  // C0 - CPY imm
    OP(IN_CMP, AM_INDX, P, CY(6)), // C1 - CMP (zp,X)
    UP(IN_NOP, AM_IMM, FOO, CY(2)),  // C2 - *NOP (DOP, SKB) imm (?)
    UP(IN_DCP, AM_INDX, FOO, CY(8)), // C3 - *DCP (DCM) (zp,X)
    OP(IN_CPY, AM_ZP, P, CY(3)),   // C4 - CPY zp
    OP(IN_CMP, AM_ZP, P, CY(3)),   // C5 - CMP zp
    OP(IN_DEC, AM_ZP, FOO, CY(5)),   // C6 - DEC zp
    UP(IN_DCP, AM_ZP, FOO, CY(5)),   // C7 - *DCP (DCM) zp
    OP(IN_INY, AM_IMP, FOO, CY(2)),  // C8 - INY
    OP(IN_CMP, AM_IMM, P, CY(2)),  // C9 - CMP imm
    OP(IN_DEX, AM_IMP, FOO, CY(2)),  // CA - DEX
    UP(IN_SBX, AM_IMM, FOO, CY(2)),  // CB - *SBX (AXS, SAX, XMA) imm
    OP(IN_CPY, AM_ABS, P, CY(4)),  // CC - CPY abs
    OP(IN_CMP, AM_ABS, P, CY(4)),  // CD - CMP abs
    OP(IN_DEC, AM_ABS, FOO, CY(6)),  // CE - DEC abs
    UP(IN_DCP, AM_ABS, FOO, CY(6)),  // CF - *DCP (DCM) abs
    OP(IN_BNE, AM_BCH, I, BR(2)),  // D0 - BNE
    OP(IN_CMP, AM_INDY, P, PG(5)), // D1 - CMP (zp),Y
    JAM,                        // D2 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_DCP, AM_INDY, FOO, CY(8)), // D3 - *DCP (DCM) (zp),Y
    UP(IN_NOP, AM_ZPX, FOO, CY(4)),  // D4 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_CMP, AM_ZPX, P, CY(4)),  // D5 - CMP zp,X
    OP(IN_DEC, AM_ZPX, FOO, CY(6)),  // D6 - DEC zp,X
    UP(IN_DCP, AM_ZPX, FOO, CY(6)),  // D7 - *DCP (DCM) zp,X
    OP(IN_CLD, AM_IMP, P, CY(2)),  // D8 - CLD
    OP(IN_CMP, AM_ABSY, P, PG(4)), // D9 - CMP abs,Y
    UP(IN_NOP, AM_IMP, FOO, CY(2)),  // DA - *NOP (NPO, UNP)
    UP(IN_DCP, AM_ABSY, FOO, CY(7)), // DB - *DCP (DCM) abs,Y
    UP(IN_NOP, AM_ABSY, FOO, PG(4)), // DC - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_CMP, AM_ABSY, P, PG(4)), // DD - CMP abs,X
    OP(IN_DEC, AM_ABSY, FOO, CY(7)), // DE - DEC abs,X
    UP(IN_DCP, AM_ABSY, FOO, CY(7)), // DF - *DCP (DCM) abs,X
    OP(IN_CPX, AM_IMM, P, CY(2)),  // E0 - CPX imm
    OP(IN_SBC, AM_INDX, AP, CY(6)), // E1 - SBC (zp,X)
    UP(IN_NOP, AM_IMM, FOO, CY(2)),  // E2 - *NOP (DOP, SKB) imm (?)
    UP(IN_ISC, AM_INDX, FOO, CY(8)), // E3 - *ISC (ISB, INS) (zp,X)
    OP(IN_CPX, AM_ZP, P, CY(3)),   // E4 - CPX zp
    OP(IN_SBC, AM_ZP, AP, CY(3)),   // E5 - SBC zp
    OP(IN_INC, AM_ZP, FOO, CY(5)),   // E6 - INC zp
    UP(IN_ISC, AM_ZP, FOO, CY(5)),   // E7 - *ISC (ISB, INS) zp
    OP(IN_INX, AM_IMP, FOO, CY(2)),  // E8 - INX
    OP(IN_SBC, AM_IMM, AP, CY(2)),  // E9 - SBC imm
    OP(IN_NOP, AM_IMP, FOO, CY(2)),  // EA - NOP
    UP(IN_SBC, AM_IMM, AP, CY(2)),  // EB - *SBC (USBC, USB) imm
    OP(IN_CPX, AM_ABS, P, CY(4)),  // EC - CPX abs
    OP(IN_SBC, AM_ABS, AP, CY(4)),  // ED - SBC abs
    OP(IN_INC, AM_ABS, FOO, CY(6)),  // EE - INC abs
    UP(IN_ISC, AM_ABS, FOO, CY(6)),  // EF - *ISC (ISB, INS) abs
    OP(IN_BEQ, AM_BCH, I, BR(2)),  // F0 - BEQ
    OP(IN_SBC, AM_INDY, AP, PG(5)), // F1 - SBC (zp),Y
    JAM,                        // F2 - *JAM (KIL, HLT, CIM, CRP)
    UP(IN_ISC, AM_INDY, FOO, CY(8)), // F3 - *ISC (ISB, INS) (zp),Y
    UP(IN_NOP, AM_ZPX, FOO, CY(4)),  // F4 - *NOP (DOP, SKB, IGN) zp,X
    OP(IN_SBC, AM_ZPX, AP, CY(4)),  // F5 - SBC zp,X
    OP(IN_INC, AM_ZPX, FOO, CY(6)),  // F6 - INC zp,X
    UP(IN_ISC, AM_ZPX, FOO, CY(6)),  // F7 - *ISC (ISB, INS) zp,X
    OP(IN_SED, AM_IMP, P, CY(2)),  // F8 - SED
    OP(IN_SBC, AM_ABSY, AP, PG(4)), // F9 - SBC abs,Y
    UP(IN_NOP, AM_IMP, FOO, CY(2)),  // FA - *NOP (NPO, UNP)
    UP(IN_ISC, AM_ABSY, FOO, CY(7)), // FB - *ISC (ISB, INS) abs,Y
    UP(IN_NOP, AM_ABSY, FOO, PG(4)), // FC - *NOP (TOP, SKW, IGN) abs,X
    OP(IN_SBC, AM_ABSY, AP, PG(4)), // FD - SBC abs,X
    OP(IN_INC, AM_ABSY, FOO, CY(7)), // FE - INC abs,X
    UP(IN_ISC, AM_ABSY, FOO, CY(7)), // FF - *ISC (ISB, INS) abs,X
};

static_assert(sizeof Decode / sizeof Decode[0] == 256,
              "Incorrect decode table size!");

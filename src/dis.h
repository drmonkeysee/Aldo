//
//  dis.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#ifndef Aldo_dis_h
#define Aldo_dis_h

#include "cart.h"
#include "cpu.h"
#include "debug.h"
#include "decode.h"
#include "snapshot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// X(symbol, value, error string)
#define DIS_ERRCODE_X \
X(DIS_ERR_FMT, -1, "FORMATTED OUTPUT FAILURE") \
X(DIS_ERR_EOF, -2, "UNEXPECTED EOF") \
X(DIS_ERR_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(DIS_ERR_CHRROM, -4, "NO CHR ROM FOUND") \
X(DIS_ERR_CHRSZ, -5, "INVALID CHR ROM SIZE") \
X(DIS_ERR_ERNO, -6, "SYSTEM ERROR") \
X(DIS_ERR_CHRSCL, -7, "INVALID CHR ROM SCALE") \
X(DIS_ERR_PRGROM, -8, "NO PRG ROM FOUND")

enum {
#define X(s, v, e) s = v,
    DIS_ERRCODE_X
#undef X
};

enum {
    DIS_OPERAND_SIZE = 8,   // Disassembled operand is at most 7 chars
    DIS_DATAP_SIZE = 12,    // Disassembled datapath is at most 11 chars
    DIS_INST_SIZE = 28,     // Disassembled instruction is at most 27 chars
    DIS_PEEK_SIZE = 20,     // Peek expression is at most 19 chars
};

struct dis_instruction {
    size_t offset;
    struct blockview bv;
    struct decoded d;
};

#include "bridgeopen.h"
//
// Export
//

br_libexport
extern const int MinChrScale, MaxChrScale;

br_libexport
const char *dis_errstr(int err);

// NOTE: parsed will be zeroed-out if return value is <= 0
br_libexport br_checkerror
int dis_parse_inst(const struct blockview *bv, size_t at,
                   struct dis_instruction *parsed);
br_libexport br_checkerror
int dis_parsemem_inst(size_t size, const uint8_t mem[br_noalias_sz(size)],
                      size_t at, struct dis_instruction *parsed);
// NOTE: functions w/buffer params leave buffer untouched when returning <= 0
br_libexport br_checkerror
int dis_inst(uint16_t addr, const struct dis_instruction *inst,
             char dis[br_noalias_csz(DIS_INST_SIZE)]);
br_libexport br_checkerror
int dis_datapath(const struct console_state *snapshot,
                 char dis[br_noalias_csz(DIS_DATAP_SIZE)]);

br_libexport br_checkerror
int dis_cart_prg(cart *cart, const char *br_noalias name, bool verbose,
                 bool unified_output, FILE *f);
br_libexport br_checkerror
int dis_cart_chr(cart *cart, int chrscale,
                 const char *br_noalias chrdecode_prefix, FILE *output);
br_libexport br_checkerror
int dis_cart_chrbank(const struct blockview *bv, int scale, FILE *f);

br_libexport
const char *dis_inst_mnemonic(const struct dis_instruction *inst);
br_libexport
const char *dis_inst_description(const struct dis_instruction *inst);
br_libexport
const char *dis_inst_addrmode(const struct dis_instruction *inst);
br_libexport
uint8_t dis_inst_flags(const struct dis_instruction *inst);
br_libexport br_checkerror
int dis_inst_operand(const struct dis_instruction *inst,
                     char dis[br_noalias_csz(DIS_OPERAND_SIZE)]);
br_libexport
bool dis_inst_equal(const struct dis_instruction *lhs,
                    const struct dis_instruction *rhs);

//
// Internal
//

int dis_peek(uint16_t addr, struct mos6502 *cpu, debugger *dbg,
             const struct console_state *snapshot,
             char dis[br_noalias_csz(DIS_PEEK_SIZE)]);
#include "bridgeclose.h"

#endif

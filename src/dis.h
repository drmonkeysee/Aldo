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
#define ALDO_DIS_ERRCODE_X \
X(DIS_ERR_FMT, -1, "FORMATTED OUTPUT FAILURE") \
X(DIS_ERR_EOF, -2, "UNEXPECTED EOF") \
X(DIS_ERR_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(DIS_ERR_CHRROM, -4, "NO CHR ROM FOUND") \
X(DIS_ERR_CHRSZ, -5, "INVALID CHR ROM SIZE") \
X(DIS_ERR_ERNO, -6, "SYSTEM ERROR") \
X(DIS_ERR_CHRSCL, -7, "INVALID CHR ROM SCALE") \
X(DIS_ERR_PRGROM, -8, "NO PRG ROM FOUND")

enum {
#define X(s, v, e) ALDO_##s = v,
    ALDO_DIS_ERRCODE_X
#undef X
};

enum {
    ALDO_DIS_OPERAND_SIZE = 8,
    ALDO_DIS_DATAP_SIZE = 12,
    ALDO_DIS_INST_SIZE = 28,
    ALDO_DIS_PEEK_SIZE = 20,
};

struct aldo_dis_instruction {
    size_t offset;
    struct blockview bv;
    struct decoded d;
};

#include "bridgeopen.h"
//
// MARK: - Export
//

br_libexport
extern const int Aldo_MinChrScale, Aldo_MaxChrScale;

br_libexport
const char *aldo_dis_errstr(int err) br_nothrow;

// NOTE: parsed will be zeroed-out if return value is <= 0
br_libexport br_checkerror
int aldo_dis_parse_inst(const struct blockview *bv, size_t at,
                        struct aldo_dis_instruction *parsed) br_nothrow;
br_libexport br_checkerror
int aldo_dis_parsemem_inst(size_t size, const uint8_t mem[br_noalias_sz(size)],
                           size_t at,
                           struct aldo_dis_instruction *parsed) br_nothrow;
// NOTE: functions w/buffer params leave buffer untouched when returning <= 0
br_libexport br_checkerror
int aldo_dis_inst(uint16_t addr, const struct aldo_dis_instruction *inst,
                  char dis[br_noalias_csz(ALDO_DIS_INST_SIZE)]) br_nothrow;
br_libexport br_checkerror
int
aldo_dis_datapath(const struct aldo_snapshot *snp,
                  char dis[br_noalias_csz(ALDO_DIS_DATAP_SIZE)]) br_nothrow;

br_libexport br_checkerror
int aldo_dis_cart_prg(cart *cart, const char *br_noalias name, bool verbose,
                      bool unified_output, FILE *f) br_nothrow;
br_libexport br_checkerror
int aldo_dis_cart_chr(cart *cart, int chrscale,
                      const char *br_noalias chrdecode_prefix,
                      FILE *output) br_nothrow;
br_libexport br_checkerror
int aldo_dis_cart_chrblock(const struct blockview *bv, int scale,
                           FILE *f) br_nothrow;

br_libexport
const char *
aldo_dis_inst_mnemonic(const struct aldo_dis_instruction *inst) br_nothrow;
br_libexport
const char *
aldo_dis_inst_description(const struct aldo_dis_instruction *inst) br_nothrow;
br_libexport
const char *
aldo_dis_inst_addrmode(const struct aldo_dis_instruction *inst) br_nothrow;
br_libexport
uint8_t
aldo_dis_inst_flags(const struct aldo_dis_instruction *inst) br_nothrow;
br_libexport br_checkerror
int
aldo_dis_inst_operand(const struct aldo_dis_instruction *inst,
                      char dis[br_noalias_csz(ALDO_DIS_OPERAND_SIZE)]) br_nothrow;
br_libexport
bool aldo_dis_inst_equal(const struct aldo_dis_instruction *lhs,
                         const struct aldo_dis_instruction *rhs) br_nothrow;

//
// MARK: - Internal
//

int aldo_dis_peek(uint16_t addr, struct mos6502 *cpu, debugger *dbg,
                  const struct aldo_snapshot *snp,
                  char dis[br_noalias_csz(ALDO_DIS_PEEK_SIZE)]) br_nothrow;
#include "bridgeclose.h"

#endif

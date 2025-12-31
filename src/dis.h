//
//  dis.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#ifndef Aldo_dis_h
#define Aldo_dis_h

#include "cart.h"
#include "debug.h"
#include "decode.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct aldo_mos6502;
struct aldo_rp2c02;
struct aldo_snapshot;

// X(symbol, value, error string)
#define ALDO_DIS_ERRCODE_X \
X(DIS_ERR_FMT, -1, "FORMATTED OUTPUT FAILURE") \
X(DIS_ERR_EOF, -2, "UNEXPECTED EOF") \
X(DIS_ERR_INV_ADDRMD, -3, "INVALID ADDRMODE") \
X(DIS_ERR_CHRROM, -4, "NO CHR ROM FOUND") \
X(DIS_ERR_CHRSZ, -5, "INVALID CHR ROM SIZE") \
X(DIS_ERR_ERNO, -6, "SYSTEM ERROR") \
X(DIS_ERR_CHRSCL, -7, "INVALID CHR ROM SCALE") \
X(DIS_ERR_PRGROM, -8, "NO PRG ROM FOUND") \
X(DIS_ERR_IO, -9, "FILE I/O ERROR")

enum {
#define X(s, v, e) ALDO_##s = v,
    ALDO_DIS_ERRCODE_X
#undef X
};

struct aldo_dis_instruction {
    size_t offset;
    struct aldo_blockview bv;
    struct aldo_decoded d;
};

#include "bridgeopen.h"
//
// MARK: - Export
//

aldo_const size_t
    AldoDisOperandSize = 8,
    AldoDisDatapSize = 12,
    AldoDisInstSize = 28;

aldo_export
extern const int Aldo_MinChrScale, Aldo_MaxChrScale;

aldo_export
const char *aldo_dis_errstr(int err) aldo_nothrow;

// NOTE: parsed will be zeroed-out if return value is <= 0
aldo_export aldo_checkerr
int aldo_dis_parse_inst(const struct aldo_blockview *bv, size_t at,
                        struct aldo_dis_instruction *parsed) aldo_nothrow;
aldo_export aldo_checkerr
int aldo_dis_parsemem_inst(size_t size, const uint8_t mem[aldo_naz(size)],
                           size_t at,
                           struct aldo_dis_instruction *parsed) aldo_nothrow;
// NOTE: functions w/buffer params leave buffer untouched when returning <= 0
aldo_export aldo_checkerr
int aldo_dis_inst(uint16_t addr, const struct aldo_dis_instruction *inst,
                  char dis[aldo_nacz(AldoDisInstSize)]) aldo_nothrow;
aldo_export aldo_checkerr
int aldo_dis_datapath(const struct aldo_snapshot *snp,
                      char dis[aldo_nacz(AldoDisDatapSize)]) aldo_nothrow;

aldo_export aldo_checkerr
int aldo_dis_cart_prg(aldo_cart *cart, const char *aldo_noalias name,
                      bool verbose, bool unified_output, FILE *f) aldo_nothrow;
aldo_export aldo_checkerr
int aldo_dis_cart_chr(aldo_cart *cart, int chrscale,
                      const char *aldo_noalias chrdecode_prefix,
                      FILE *output) aldo_nothrow;
aldo_export aldo_checkerr
int aldo_dis_cart_chrblock(const struct aldo_blockview *bv, int scale,
                           FILE *f) aldo_nothrow;

aldo_export
const char *
aldo_dis_inst_mnemonic(const struct aldo_dis_instruction *inst) aldo_nothrow;
aldo_export
const char *
aldo_dis_inst_description(const struct aldo_dis_instruction *
                          inst) aldo_nothrow;
aldo_export
const char *
aldo_dis_inst_addrmode(const struct aldo_dis_instruction *inst) aldo_nothrow;
aldo_export
uint8_t
aldo_dis_inst_flags(const struct aldo_dis_instruction *inst) aldo_nothrow;
aldo_export aldo_checkerr
int
aldo_dis_inst_operand(const struct aldo_dis_instruction *inst,
                      char dis[aldo_nacz(AldoDisOperandSize)]) aldo_nothrow;
aldo_export
bool aldo_dis_inst_equal(const struct aldo_dis_instruction *lhs,
                         const struct aldo_dis_instruction *rhs) aldo_nothrow;

//
// MARK: - Internal
//
aldo_const size_t AldoDisPeekSize = 20;

int aldo_dis_peek(struct aldo_mos6502 *cpu, struct aldo_rp2c02 *ppu,
                  aldo_debugger *dbg, const struct aldo_snapshot *snp,
                  char dis[aldo_nacz(AldoDisPeekSize)]) aldo_nothrow;
#include "bridgeclose.h"

#endif

//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"
#include "ctrlsignal.h"
#include "debug.h"
#include "decode.h"
#include "dis.h"
#include "snapshot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define makeinst(b) create_instruction(sizeof (b) / sizeof (b)[0], b)
static struct dis_instruction create_instruction(size_t sz,
                                                 const uint8_t bytes[sz])
{
    struct blockview bv = {.mem = bytes, .size = sz};
    struct dis_instruction inst;
    int err = dis_parse_inst(&bv, 0, &inst);
    ct_asserttrue(err > 0);
    return inst;
}

//
// MARK: - Error Strings
//

static void errstr_returns_known_err(void *ctx)
{
    const char *err = dis_errstr(DIS_ERR_FMT);

    ct_assertequalstr("FORMATTED OUTPUT FAILURE", err);
}

static void errstr_returns_unknown_err(void *ctx)
{
    const char *err = dis_errstr(10);

    ct_assertequalstr("UNKNOWN ERR", err);
}

//
// MARK: - Parse Instructions
//

static void parse_inst_empty_bankview(void *ctx)
{
    struct blockview bv = {.size = 0};
    struct dis_instruction inst;

    int result = dis_parse_inst(&bv, 0, &inst);

    ct_assertequal(DIS_ERR_PRGROM, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertnull(inst.bv.mem);
    ct_assertequal(0u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_UDF, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parse_inst_at_start(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    int result = dis_parse_inst(&bv, 0, &inst);

    ct_assertequal(1, result);
    ct_assertequal(bv.ord, inst.bv.ord);
    ct_assertequal(0xeau, inst.bv.mem[0]);
    ct_assertequal(1u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_NOP, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parse_inst_in_middle(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    int result = dis_parse_inst(&bv, 3, &inst);

    ct_assertequal(3, result);
    ct_assertequal(bv.ord, inst.bv.ord);
    ct_assertequal(0x4cu, inst.bv.mem[0]);
    ct_assertequal(0x34u, inst.bv.mem[1]);
    ct_assertequal(0x6u, inst.bv.mem[2]);
    ct_assertequal(3u, inst.bv.size);
    ct_assertequal(3u, inst.offset);
    ct_assertequal(IN_JMP, (int)inst.d.instruction);
    ct_assertequal(AM_JABS, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parse_inst_unofficial(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    int result = dis_parse_inst(&bv, 2, &inst);

    ct_assertequal(2, result);
    ct_assertequal(bv.ord, inst.bv.ord);
    ct_assertequal(0x34u, inst.bv.mem[0]);
    ct_assertequal(0x4cu, inst.bv.mem[1]);
    ct_assertequal(2u, inst.bv.size);
    ct_assertequal(2u, inst.offset);
    ct_assertequal(IN_NOP, (int)inst.d.instruction);
    ct_assertequal(AM_ZPX, (int)inst.d.mode);
    ct_asserttrue(inst.d.unofficial);
}

static void parse_inst_eof(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    int result = dis_parse_inst(&bv, 5, &inst);

    ct_assertequal(DIS_ERR_EOF, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertnull(inst.bv.mem);
    ct_assertequal(0u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_UDF, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parse_inst_out_of_bounds(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    int result = dis_parse_inst(&bv, 10, &inst);

    ct_assertequal(0, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertnull(inst.bv.mem);
    ct_assertequal(0u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_UDF, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parsemem_inst_empty_blockview(void *ctx)
{
    struct dis_instruction inst;

    int result = dis_parsemem_inst(0, NULL, 0, &inst);

    ct_assertequal(DIS_ERR_PRGROM, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertnull(inst.bv.mem);
    ct_assertequal(0u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_UDF, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parsemem_inst_at_start(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 0, &inst);

    ct_assertequal(1, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertequal(0xeau, inst.bv.mem[0]);
    ct_assertequal(1u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_NOP, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parsemem_inst_in_middle(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 3, &inst);

    ct_assertequal(3, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertequal(0x4cu, inst.bv.mem[0]);
    ct_assertequal(0x34u, inst.bv.mem[1]);
    ct_assertequal(0x6u, inst.bv.mem[2]);
    ct_assertequal(3u, inst.bv.size);
    ct_assertequal(3u, inst.offset);
    ct_assertequal(IN_JMP, (int)inst.d.instruction);
    ct_assertequal(AM_JABS, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parsemem_inst_unofficial(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 2, &inst);

    ct_assertequal(2, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertequal(0x34u, inst.bv.mem[0]);
    ct_assertequal(0x4cu, inst.bv.mem[1]);
    ct_assertequal(2u, inst.bv.size);
    ct_assertequal(2u, inst.offset);
    ct_assertequal(IN_NOP, (int)inst.d.instruction);
    ct_assertequal(AM_ZPX, (int)inst.d.mode);
    ct_asserttrue(inst.d.unofficial);
}

static void parsemem_inst_eof(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 5, &inst);

    ct_assertequal(DIS_ERR_EOF, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertnull(inst.bv.mem);
    ct_assertequal(0u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_UDF, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

static void parsemem_inst_out_of_bounds(void *ctx)
{
    uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 10, &inst);

    ct_assertequal(0, result);
    ct_assertequal(0u, inst.bv.ord);
    ct_assertnull(inst.bv.mem);
    ct_assertequal(0u, inst.bv.size);
    ct_assertequal(0u, inst.offset);
    ct_assertequal(IN_UDF, (int)inst.d.instruction);
    ct_assertequal(AM_IMP, (int)inst.d.mode);
    ct_assertfalse(inst.d.unofficial);
}

//
// MARK: - Mnemonics
//

static void mnemonic_valid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ADC, AM_IMM, {0}, {0}, false},
    };

    const char *result = dis_inst_mnemonic(&inst);

    ct_assertequalstr("ADC", result);
}

static void mnemonic_unofficial(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ANC, AM_IMM, {0}, {0}, true},
    };

    const char *result = dis_inst_mnemonic(&inst);

    ct_assertequalstr("ANC", result);
}

static void mnemonic_invalid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {(enum inst)-4, AM_IMM, {0}, {0}, false},
    };

    const char *result = dis_inst_mnemonic(&inst);

    ct_assertequalstr("UDF", result);
}

//
// MARK: - Descriptions
//

static void description_valid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ADC, AM_IMM, {0}, {0}, false},
    };

    const char *result = dis_inst_description(&inst);

    ct_assertequalstr("Add with carry", result);
}

static void description_unofficial(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ANC, AM_IMM, {0}, {0}, true},
    };

    const char *result = dis_inst_description(&inst);

    ct_assertequalstr("AND + set carry as if ASL or ROL", result);
}

static void description_invalid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {(enum inst)-4, AM_IMM, {0}, {0}, false},
    };

    const char *result = dis_inst_description(&inst);

    ct_assertequalstr("Undefined", result);
}

//
// MARK: - Address Mode Names
//

static void modename_valid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ADC, AM_ZP, {0}, {0}, false},
    };

    const char *result = dis_inst_addrmode(&inst);

    ct_assertequalstr("Zero-Page", result);
}

static void modename_unofficial(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ADC, AM_JAM, {0}, {0}, true},
    };

    const char *result = dis_inst_addrmode(&inst);

    ct_assertequalstr("Implied", result);
}

static void modename_invalid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ADC, (enum addrmode)-4, {0}, {0}, false},
    };

    const char *result = dis_inst_addrmode(&inst);

    ct_assertequalstr("Implied", result);
}

//
// MARK: - Flags
//

static void flags_valid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ADC, AM_IMM, {0}, {0}, false},
    };

    uint8_t result = dis_inst_flags(&inst);

    ct_assertequal(0xe3u, result);
}

static void flags_unofficial(void *ctx)
{
    struct dis_instruction inst = {
        .d = {IN_ANC, AM_IMM, {0}, {0}, true},
    };

    uint8_t result = dis_inst_flags(&inst);

    ct_assertequal(0xa3u, result);
}

static void flags_invalid(void *ctx)
{
    struct dis_instruction inst = {
        .d = {(enum inst)-4, AM_IMM, {0}, {0}, false},
    };

    uint8_t result = dis_inst_flags(&inst);

    ct_assertequal(0x20u, result);
}

//
// MARK: - Instruction Operand
//

static void inst_operand_empty_instruction(void *ctx)
{
    struct dis_instruction inst = {0};
    char buf[DIS_OPERAND_SIZE];

    int length = dis_inst_operand(&inst, buf);

    const char *exp = "";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_operand_no_operand(void *ctx)
{
    uint8_t mem[] = {0xea};
    struct dis_instruction inst = makeinst(mem);
    char buf[DIS_OPERAND_SIZE];

    int length = dis_inst_operand(&inst, buf);

    const char *exp = "";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_operand_one_byte_operand(void *ctx)
{
    uint8_t mem[] = {0x65, 0x6};
    struct dis_instruction inst = makeinst(mem);
    char buf[DIS_OPERAND_SIZE];

    int length = dis_inst_operand(&inst, buf);

    const char *exp = "$06";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_operand_two_byte_operand(void *ctx)
{
    uint8_t mem[] = {0xad, 0x34, 0x4c};
    struct dis_instruction inst = makeinst(mem);
    char buf[DIS_OPERAND_SIZE];

    int length = dis_inst_operand(&inst, buf);

    const char *exp = "$4C34";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// MARK: - Instruction Equality
//

static void inst_eq_both_are_null(void *ctx)
{
    bool result = dis_inst_equal(NULL, NULL);

    ct_assertfalse(result);
}

static void inst_eq_rhs_is_null(void *ctx)
{
    uint8_t a[] = {0xea};
    struct dis_instruction lhs = makeinst(a);

    bool result = dis_inst_equal(&lhs, NULL);

    ct_assertfalse(result);
}

static void inst_eq_lhs_is_null(void *ctx)
{
    uint8_t b[] = {0xad, 0x34, 0x4c};
    struct dis_instruction rhs = makeinst(b);

    bool result = dis_inst_equal(NULL, &rhs);

    ct_assertfalse(result);
}

static void inst_eq_different_lengths(void *ctx)
{
    uint8_t a[] = {0xea}, b[] = {0xad, 0x34, 0x4c};
    struct dis_instruction lhs = makeinst(a), rhs = makeinst(b);

    bool result = dis_inst_equal(&lhs, &rhs);

    ct_assertfalse(result);
}

static void inst_eq_different_bytes(void *ctx)
{
    uint8_t a[] = {0xad, 0x44, 0x80}, b[] = {0xad, 0x34, 0x4c};
    struct dis_instruction lhs = makeinst(a), rhs = makeinst(b);

    bool result = dis_inst_equal(&lhs, &rhs);

    ct_assertfalse(result);
}

static void inst_eq_same_bytes(void *ctx)
{
    uint8_t a[] = {0xad, 0x34, 0x4c}, b[] = {0xad, 0x34, 0x4c};
    struct dis_instruction lhs = makeinst(a), rhs = makeinst(b);

    bool result = dis_inst_equal(&lhs, &rhs);

    ct_asserttrue(result);
}

static void inst_eq_same_object(void *ctx)
{
    uint8_t a[] = {0xad, 0x44, 0x80};
    struct dis_instruction lhs = makeinst(a);

    bool result = dis_inst_equal(&lhs, &lhs);

    ct_asserttrue(result);
}

//
// MARK: - Disassemble Instruction
//

static void inst_does_nothing_if_no_bytes(void *ctx)
{
    uint16_t a = 0x1234;
    struct dis_instruction inst = {0};
    char buf[DIS_INST_SIZE] = {'\0'};

    int length = dis_inst(a, &inst, buf);

    const char *exp = "";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_implied(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xea};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: EA        NOP";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_immediate(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xa9, 0x34};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: A9 34     LDA #$34";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_zeropage(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xa5, 0x34};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: A5 34     LDA $34";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_zeropage_x(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xb5, 0x34};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: B5 34     LDA $34,X";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_zeropage_y(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xb6, 0x34};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: B6 34     LDX $34,Y";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_indirect_x(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xa1, 0x34};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: A1 34     LDA ($34,X)";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_indirect_y(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xb1, 0x34};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: B1 34     LDA ($34),Y";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_absolute(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xad, 0x34, 0x6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: AD 34 06  LDA $0634";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_absolute_x(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xbd, 0x34, 0x6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: BD 34 06  LDA $0634,X";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_absolute_y(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0xb9, 0x34, 0x6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: B9 34 06  LDA $0634,Y";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_jmp_absolute(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x4c, 0x34, 0x6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 4C 34 06  JMP $0634";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_jmp_indirect(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x6c, 0x34, 0x6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 6C 34 06  JMP ($0634)";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_branch_positive(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x90, 0xa};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 90 0A     BCC +10";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_branch_negative(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x90, 0xf6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 90 F6     BCC -10";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_branch_zero(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x90, 0x0};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 90 00     BCC +0";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_push(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x48};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 48        PHA";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_pull(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x68};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 68        PLA";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_jsr(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x20, 0x34, 0x6};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 20 34 06  JSR $0634";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_rts(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x60};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 60        RTS";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_brk(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x0};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 00        BRK";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_unofficial(void *ctx)
{
    uint16_t a = 0x1234;
    uint8_t bytes[] = {0x02};
    struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    int length = dis_inst(a, &inst, buf);

    const char *exp = "1234: 02       *JAM";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// MARK: - Disassemble Datapath
//

static void datapath_end_of_rom(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea},
        .length = 1,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_unexpected_end_of_rom(void *ctx)
{
    // NOTE: LDA imm with missing 2nd byte
    struct snpprg curr = {
        .pc = {0xa9},
        .length = 1,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE] = {'\0'};

    int written = dis_datapath(&snp, buf);

    const char *exp = "";
    ct_assertequal(DIS_ERR_EOF, written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_implied_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "NOP imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_implied_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_implied_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_immediate_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa9, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA imm";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_immediate_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa9, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA #$43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_immediate_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa9, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA #$43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa5, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA zp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa5, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa5, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_x_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb5, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA zp,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_x_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb5, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_x_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb5, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_y_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb6, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDX zp,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_y_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb6, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDX $43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_y_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb6, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDX $43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_x_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa1, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA (zp,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_x_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa1, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA ($43,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_x_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xa1, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA ($43,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_y_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb1, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA (zp),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_y_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb1, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA ($43),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_y_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb1, 0x43},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA ($43),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xad, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xad, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_two(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xad, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xad, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 3,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xbd, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA abs,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xbd, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $??43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_two(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xbd, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $2143,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xbd, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 3,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $2143,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb9, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA abs,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb9, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $??43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_two(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb9, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $2143,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xb9, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 3,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "LDA $2143,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x4c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x4c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_two(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x4c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x4c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 3,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x6c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP (abs)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x6c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP ($??43)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_two(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x6c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP ($2143)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x6c, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 3,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JMP ($2143)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_bch_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x90, 0x2, 0xff, 0xff, 0xff},
        .length = 5,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BCC rel";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_bch_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x90, 0x2, 0xff, 0xff, 0xff},
        .length = 5,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BCC +2";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_bch_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x90, 0x2, 0xff, 0xff, 0xff},
        .length = 5,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BCC +2";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_push_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x48, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "PHA imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_push_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x48, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "PHA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_push_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x48, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "PHA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_pull_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x68, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "PLA imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_pull_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x68, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "PLA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_pull_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x68, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "PLA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x20, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JSR abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x20, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JSR $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_two(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x20, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JSR $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x20, 0x43, 0x21},
        .length = 3,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 3,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "JSR $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rts_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x60, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "RTS imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rts_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x60, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "RTS ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rts_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x60, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "RTS ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x0, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 0,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x0, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 1,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x0, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 2,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_six(void *ctx)
{
    struct snpprg curr = {
        .pc = {0x0, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath.exec_cycle = 6,
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = snp.prg.curr->pc[0];
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 0,
            .irq = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 1,
            .irq = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK (IRQ)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 2,
            .irq = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK (IRQ)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_six(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 6,
            .irq = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 0,
            .nmi = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 1,
            .nmi = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK (NMI)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 2,
            .nmi = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK (NMI)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_six(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 6,
            .nmi = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rst_cycle_zero(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 0,
            .rst = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rst_cycle_one(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 1,
            .rst = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK (RST)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rst_cycle_n(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 2,
            .rst = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK (RST)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rst_cycle_six(void *ctx)
{
    struct snpprg curr = {
        .pc = {0xea, 0xff},
        .length = 2,
    };
    struct snapshot snp = {
        .cpu.datapath = {
            .exec_cycle = 6,
            .rst = CSGS_COMMITTED,
        },
        .prg.curr = &curr,
    };
    snp.cpu.datapath.opcode = Aldo_BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    int written = dis_datapath(&snp, buf);

    const char *exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// MARK: - Disassemble Peek
//

static void setup_peek(void **ctx)
{
    *ctx = debug_new();
}

static void teardown_peek(void **ctx)
{
    debug_free(*ctx);
}

static void peek_immediate(void *ctx)
{
    // NOTE: LDA #$10
    uint8_t mem[] = {0xa9, 0x10};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    setup_cpu(&cpu, mem, NULL);
    struct snapshot snp;
    cpu.a = 0x10;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_zeropage(void *ctx)
{
    // NOTE: LDA $04
    uint8_t mem[] = {0xa5, 0x4, 0x0, 0x0, 0x20};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    setup_cpu(&cpu, mem, NULL);
    struct snapshot snp;
    cpu.a = 0x10;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "= 20";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_zp_indexed(void *ctx)
{
    // NOTE: LDA $03,X
    uint8_t mem[] = {0xb5, 0x3, 0x0, 0x0, 0x0, 0x30};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 2;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "@ 05 = 30";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_indexed_indirect(void *ctx)
{
    // NOTE: LDA ($02,X)
    uint8_t mem[] = {0xa1, 0x2, 0x0, 0x0, 0x2, 0x1, [258] = 0x40};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 2;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "@ 04 > 0102 = 40";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_indirect_indexed(void *ctx)
{
    // NOTE: LDA ($02),Y
    uint8_t mem[] = {0xb1, 0x2, 0x2, 0x1, [263] = 0x60};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.y = 5;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "> 0102 @ 0107 = 60";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_absolute_indexed(void *ctx)
{
    // NOTE: LDA $0102,X
    uint8_t mem[] = {0xbd, 0x2, 0x1, [268] = 0x70};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 0xa;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "@ 010C = 70";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_branch(void *ctx)
{
    // NOTE: BEQ +5
    uint8_t mem[] = {0xf0, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x55};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "@ 0007";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_branch_forced(void *ctx)
{
    // NOTE: BEQ +5
    uint8_t mem[] = {0xf0, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x55};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = false;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "@ 0007";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_absolute_indirect(void *ctx)
{
    // NOTE: LDA ($0102)
    uint8_t mem[] = {0x6c, 0x2, 0x1, [258] = 0x5, 0x2, [517] = 80};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 0xa;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "> 0205";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_interrupt(void *ctx)
{
    // NOTE: LDA $04
    uint8_t mem[] = {0xa5, 0x4, 0x0, 0x0, 0x20};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    setup_cpu(&cpu, mem, NULL);
    struct snapshot snp;
    cpu.a = 0x10;
    cpu.irq = CSGS_COMMITTED;
    cpu_snapshot(&cpu, &snp);
    debugger *dbg = ctx;
    debug_set_vector_override(dbg, Aldo_NoResetVector);
    snp.prg.vectors[4] = 0xbb;
    snp.prg.vectors[5] = 0xaa;

    int written = dis_peek(0x0, &cpu, dbg, &snp, buf);

    const char *exp = "(IRQ) > AABB";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_overridden_reset(void *ctx)
{
    // NOTE: LDA $04
    uint8_t mem[] = {0xa5, 0x4, 0x0, 0x0, 0x20};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    setup_cpu(&cpu, mem, NULL);
    struct snapshot snp;
    cpu.a = 0x10;
    cpu.rst = CSGS_COMMITTED;
    cpu_snapshot(&cpu, &snp);
    debugger *dbg = ctx;
    debug_set_vector_override(dbg, 0xccdd);
    snp.prg.vectors[2] = 0xbb;
    snp.prg.vectors[3] = 0xaa;

    int written = dis_peek(0x0, &cpu, dbg, &snp, buf);

    const char *exp = "(RST) > !CCDD";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_overridden_non_reset(void *ctx)
{
    // NOTE: LDA $04
    uint8_t mem[] = {0xa5, 0x4, 0x0, 0x0, 0x20};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    setup_cpu(&cpu, mem, NULL);
    struct snapshot snp;
    cpu.a = 0x10;
    cpu.nmi = CSGS_COMMITTED;
    cpu_snapshot(&cpu, &snp);
    debugger *dbg = ctx;
    debug_set_vector_override(dbg, 0xccdd);
    snp.prg.vectors[0] = 0xff;
    snp.prg.vectors[1] = 0xee;
    snp.prg.vectors[2] = 0xbb;
    snp.prg.vectors[3] = 0xaa;

    int written = dis_peek(0x0, &cpu, dbg, &snp, buf);

    const char *exp = "(NMI) > EEFF";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

static void peek_busfault(void *ctx)
{
    // NOTE: LDA ($02),Y
    uint8_t mem[] = {0xb1, 0x2, 0x2, 0x40};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    struct snapshot snp;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.y = 5;
    cpu_snapshot(&cpu, &snp);

    int written = dis_peek(0x0, &cpu, ctx, &snp, buf);

    const char *exp = "> 4002 @ 4007 = FLT";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

//
// MARK: - Test List
//

struct ct_testsuite dis_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(errstr_returns_known_err),
        ct_maketest(errstr_returns_unknown_err),

        ct_maketest(parse_inst_empty_bankview),
        ct_maketest(parse_inst_at_start),
        ct_maketest(parse_inst_in_middle),
        ct_maketest(parse_inst_unofficial),
        ct_maketest(parse_inst_eof),
        ct_maketest(parse_inst_out_of_bounds),
        ct_maketest(parsemem_inst_empty_blockview),
        ct_maketest(parsemem_inst_at_start),
        ct_maketest(parsemem_inst_in_middle),
        ct_maketest(parsemem_inst_unofficial),
        ct_maketest(parsemem_inst_eof),
        ct_maketest(parsemem_inst_out_of_bounds),

        ct_maketest(mnemonic_valid),
        ct_maketest(mnemonic_unofficial),
        ct_maketest(mnemonic_invalid),

        ct_maketest(description_valid),
        ct_maketest(description_unofficial),
        ct_maketest(description_invalid),

        ct_maketest(modename_valid),
        ct_maketest(modename_unofficial),
        ct_maketest(modename_invalid),

        ct_maketest(flags_valid),
        ct_maketest(flags_unofficial),
        ct_maketest(flags_invalid),

        ct_maketest(inst_operand_empty_instruction),
        ct_maketest(inst_operand_no_operand),
        ct_maketest(inst_operand_one_byte_operand),
        ct_maketest(inst_operand_two_byte_operand),

        ct_maketest(inst_eq_both_are_null),
        ct_maketest(inst_eq_rhs_is_null),
        ct_maketest(inst_eq_lhs_is_null),
        ct_maketest(inst_eq_different_lengths),
        ct_maketest(inst_eq_different_bytes),
        ct_maketest(inst_eq_same_bytes),
        ct_maketest(inst_eq_same_object),

        ct_maketest(inst_does_nothing_if_no_bytes),
        ct_maketest(inst_disassembles_implied),
        ct_maketest(inst_disassembles_immediate),
        ct_maketest(inst_disassembles_zeropage),
        ct_maketest(inst_disassembles_zeropage_x),
        ct_maketest(inst_disassembles_zeropage_y),
        ct_maketest(inst_disassembles_indirect_x),
        ct_maketest(inst_disassembles_indirect_y),
        ct_maketest(inst_disassembles_absolute),
        ct_maketest(inst_disassembles_absolute_x),
        ct_maketest(inst_disassembles_absolute_y),
        ct_maketest(inst_disassembles_jmp_absolute),
        ct_maketest(inst_disassembles_jmp_indirect),
        ct_maketest(inst_disassembles_branch_positive),
        ct_maketest(inst_disassembles_branch_negative),
        ct_maketest(inst_disassembles_branch_zero),
        ct_maketest(inst_disassembles_push),
        ct_maketest(inst_disassembles_pull),
        ct_maketest(inst_disassembles_jsr),
        ct_maketest(inst_disassembles_rts),
        ct_maketest(inst_disassembles_brk),
        ct_maketest(inst_disassembles_unofficial),

        ct_maketest(datapath_end_of_rom),
        ct_maketest(datapath_unexpected_end_of_rom),

        ct_maketest(datapath_implied_cycle_zero),
        ct_maketest(datapath_implied_cycle_one),
        ct_maketest(datapath_implied_cycle_n),

        ct_maketest(datapath_immediate_cycle_zero),
        ct_maketest(datapath_immediate_cycle_one),
        ct_maketest(datapath_immediate_cycle_n),

        ct_maketest(datapath_zeropage_cycle_zero),
        ct_maketest(datapath_zeropage_cycle_one),
        ct_maketest(datapath_zeropage_cycle_n),

        ct_maketest(datapath_zeropage_x_cycle_zero),
        ct_maketest(datapath_zeropage_x_cycle_one),
        ct_maketest(datapath_zeropage_x_cycle_n),

        ct_maketest(datapath_zeropage_y_cycle_zero),
        ct_maketest(datapath_zeropage_y_cycle_one),
        ct_maketest(datapath_zeropage_y_cycle_n),

        ct_maketest(datapath_indirect_x_cycle_zero),
        ct_maketest(datapath_indirect_x_cycle_one),
        ct_maketest(datapath_indirect_x_cycle_n),

        ct_maketest(datapath_indirect_y_cycle_zero),
        ct_maketest(datapath_indirect_y_cycle_one),
        ct_maketest(datapath_indirect_y_cycle_n),

        ct_maketest(datapath_absolute_cycle_zero),
        ct_maketest(datapath_absolute_cycle_one),
        ct_maketest(datapath_absolute_cycle_two),
        ct_maketest(datapath_absolute_cycle_n),

        ct_maketest(datapath_absolute_x_cycle_zero),
        ct_maketest(datapath_absolute_x_cycle_one),
        ct_maketest(datapath_absolute_x_cycle_two),
        ct_maketest(datapath_absolute_x_cycle_n),

        ct_maketest(datapath_absolute_y_cycle_zero),
        ct_maketest(datapath_absolute_y_cycle_one),
        ct_maketest(datapath_absolute_y_cycle_two),
        ct_maketest(datapath_absolute_y_cycle_n),

        ct_maketest(datapath_jmp_absolute_cycle_zero),
        ct_maketest(datapath_jmp_absolute_cycle_one),
        ct_maketest(datapath_jmp_absolute_cycle_two),
        ct_maketest(datapath_jmp_absolute_cycle_n),

        ct_maketest(datapath_jmp_indirect_cycle_zero),
        ct_maketest(datapath_jmp_indirect_cycle_one),
        ct_maketest(datapath_jmp_indirect_cycle_two),
        ct_maketest(datapath_jmp_indirect_cycle_n),

        ct_maketest(datapath_bch_cycle_zero),
        ct_maketest(datapath_bch_cycle_one),
        ct_maketest(datapath_bch_cycle_n),

        ct_maketest(datapath_push_cycle_zero),
        ct_maketest(datapath_push_cycle_one),
        ct_maketest(datapath_push_cycle_n),

        ct_maketest(datapath_pull_cycle_zero),
        ct_maketest(datapath_pull_cycle_one),
        ct_maketest(datapath_pull_cycle_n),

        ct_maketest(datapath_jsr_cycle_zero),
        ct_maketest(datapath_jsr_cycle_one),
        ct_maketest(datapath_jsr_cycle_two),
        ct_maketest(datapath_jsr_cycle_n),

        ct_maketest(datapath_rts_cycle_zero),
        ct_maketest(datapath_rts_cycle_one),
        ct_maketest(datapath_rts_cycle_n),

        ct_maketest(datapath_brk_cycle_zero),
        ct_maketest(datapath_brk_cycle_one),
        ct_maketest(datapath_brk_cycle_n),
        ct_maketest(datapath_brk_cycle_six),

        ct_maketest(datapath_irq_cycle_zero),
        ct_maketest(datapath_irq_cycle_one),
        ct_maketest(datapath_irq_cycle_n),
        ct_maketest(datapath_irq_cycle_six),

        ct_maketest(datapath_nmi_cycle_zero),
        ct_maketest(datapath_nmi_cycle_one),
        ct_maketest(datapath_nmi_cycle_n),
        ct_maketest(datapath_nmi_cycle_six),

        ct_maketest(datapath_rst_cycle_zero),
        ct_maketest(datapath_rst_cycle_one),
        ct_maketest(datapath_rst_cycle_n),
        ct_maketest(datapath_rst_cycle_six),
    };

    return ct_makesuite(tests);
}

struct ct_testsuite dis_peek_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(peek_immediate),
        ct_maketest(peek_zeropage),
        ct_maketest(peek_zp_indexed),
        ct_maketest(peek_indexed_indirect),
        ct_maketest(peek_indirect_indexed),
        ct_maketest(peek_absolute_indexed),
        ct_maketest(peek_branch),
        ct_maketest(peek_branch_forced),
        ct_maketest(peek_absolute_indirect),
        ct_maketest(peek_interrupt),
        ct_maketest(peek_overridden_reset),
        ct_maketest(peek_overridden_non_reset),
        ct_maketest(peek_busfault),
    };

    return ct_makesuite_setup_teardown(tests, setup_peek, teardown_peek);
}

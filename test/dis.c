//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"
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
    const int err = dis_parse_inst(&bv, 0, &inst);
    ct_asserttrue(err > 0);
    return inst;
}

//
// Error Strings
//

static void errstr_returns_known_err(void *ctx)
{
    const char *const err = dis_errstr(DIS_ERR_FMT);

    ct_assertequalstr("FORMATTED OUTPUT FAILURE", err);
}

static void errstr_returns_unknown_err(void *ctx)
{
    const char *const err = dis_errstr(10);

    ct_assertequalstr("UNKNOWN ERR", err);
}

//
// Parse Instructions
//

static void parse_inst_empty_bankview(void *ctx)
{
    struct blockview bv = {.size = 0};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 0, &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 0, &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 3, &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 2, &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 5, &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct blockview bv = {
        .mem = mem,
        .size = sizeof mem / sizeof mem[0],
        .ord = 1,
    };
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 10, &inst);

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

    const int result = dis_parsemem_inst(0, NULL, 0, &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    const int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 0,
                                         &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    const int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 3,
                                         &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    const int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 2,
                                         &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    const int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 5,
                                         &inst);

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
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct dis_instruction inst;

    const int result = dis_parsemem_inst(sizeof mem / sizeof mem[0], mem, 10,
                                         &inst);

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
// Mnemonics
//

static void mnemonic_valid(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ADC, AM_IMM, {0}, {0}, false},
    };

    const char *const result = dis_inst_mnemonic(&inst);

    ct_assertequalstr("ADC", result);
}

static void mnemonic_unofficial(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ANC, AM_IMM, {0}, {0}, true},
    };

    const char *const result = dis_inst_mnemonic(&inst);

    ct_assertequalstr("ANC", result);
}

static void mnemonic_invalid(void *ctx)
{
    const struct dis_instruction inst = {.d = {-4, AM_IMM, {0}, {0}, false}};

    const char *const result = dis_inst_mnemonic(&inst);

    ct_assertequalstr("UDF", result);
}

//
// Descriptions
//

static void description_valid(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ADC, AM_IMM, {0}, {0}, false},
    };

    const char *const result = dis_inst_description(&inst);

    ct_assertequalstr("Add with carry", result);
}

static void description_unofficial(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ANC, AM_IMM, {0}, {0}, true},
    };

    const char *const result = dis_inst_description(&inst);

    ct_assertequalstr("AND + set carry as if ASL or ROL", result);
}

static void description_invalid(void *ctx)
{
    const struct dis_instruction inst = {.d = {-4, AM_IMM, {0}, {0}, false}};

    const char *const result = dis_inst_description(&inst);

    ct_assertequalstr("Undefined", result);
}

//
// Address Mode Names
//

static void modename_valid(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ADC, AM_ZP, {0}, {0}, false},
    };

    const char *const result = dis_inst_addrmode(&inst);

    ct_assertequalstr("Zero-Page", result);
}

static void modename_unofficial(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ADC, AM_JAM, {0}, {0}, true},
    };

    const char *const result = dis_inst_addrmode(&inst);

    ct_assertequalstr("Implied", result);
}

static void modename_invalid(void *ctx)
{
    const struct dis_instruction inst = {.d = {IN_ADC, -4, {0}, {0}, false}};

    const char *const result = dis_inst_addrmode(&inst);

    ct_assertequalstr("Implied", result);
}

//
// Flags
//

static void flags_valid(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ADC, AM_IMM, {0}, {0}, false},
    };

    const uint8_t result = dis_inst_flags(&inst);

    ct_assertequal(0xe3u, result);
}

static void flags_unofficial(void *ctx)
{
    const struct dis_instruction inst = {
        .d = {IN_ANC, AM_IMM, {0}, {0}, true},
    };

    const uint8_t result = dis_inst_flags(&inst);

    ct_assertequal(0xa3u, result);
}

static void flags_invalid(void *ctx)
{
    const struct dis_instruction inst = {.d = {-4, AM_IMM, {0}, {0}, false}};

    const uint8_t result = dis_inst_flags(&inst);

    ct_assertequal(0x20u, result);
}

//
// Instruction Operand
//

static void inst_operand_empty_instruction(void *ctx)
{
    const struct dis_instruction inst = {0};
    char buf[DIS_OPERAND_SIZE];

    const int length = dis_inst_operand(&inst, buf);

    const char *const exp = "";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_operand_no_operand(void *ctx)
{
    const uint8_t mem[] = {0xea};
    const struct dis_instruction inst = makeinst(mem);
    char buf[DIS_OPERAND_SIZE];

    const int length = dis_inst_operand(&inst, buf);

    const char *const exp = "";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_operand_one_byte_operand(void *ctx)
{
    const uint8_t mem[] = {0x65, 0x6};
    const struct dis_instruction inst = makeinst(mem);
    char buf[DIS_OPERAND_SIZE];

    const int length = dis_inst_operand(&inst, buf);

    const char *const exp = "$06";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_operand_two_byte_operand(void *ctx)
{
    const uint8_t mem[] = {0xad, 0x34, 0x4c};
    const struct dis_instruction inst = makeinst(mem);
    char buf[DIS_OPERAND_SIZE];

    const int length = dis_inst_operand(&inst, buf);

    const char *const exp = "$4C34";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// Instruction Equality
//

static void inst_eq_both_are_null(void *ctx)
{
    const bool result = dis_inst_equal(NULL, NULL);

    ct_assertfalse(result);
}

static void inst_eq_rhs_is_null(void *ctx)
{
    const uint8_t a[] = {0xea};
    const struct dis_instruction lhs = makeinst(a);

    const bool result = dis_inst_equal(&lhs, NULL);

    ct_assertfalse(result);
}

static void inst_eq_lhs_is_null(void *ctx)
{
    const uint8_t b[] = {0xad, 0x34, 0x4c};
    const struct dis_instruction rhs = makeinst(b);

    const bool result = dis_inst_equal(NULL, &rhs);

    ct_assertfalse(result);
}

static void inst_eq_different_lengths(void *ctx)
{
    const uint8_t a[] = {0xea}, b[] = {0xad, 0x34, 0x4c};
    const struct dis_instruction lhs = makeinst(a), rhs = makeinst(b);

    const bool result = dis_inst_equal(&lhs, &rhs);

    ct_assertfalse(result);
}

static void inst_eq_different_bytes(void *ctx)
{
    const uint8_t a[] = {0xad, 0x44, 0x80}, b[] = {0xad, 0x34, 0x4c};
    const struct dis_instruction lhs = makeinst(a), rhs = makeinst(b);

    const bool result = dis_inst_equal(&lhs, &rhs);

    ct_assertfalse(result);
}

static void inst_eq_same_bytes(void *ctx)
{
    const uint8_t a[] = {0xad, 0x34, 0x4c}, b[] = {0xad, 0x34, 0x4c};
    const struct dis_instruction lhs = makeinst(a), rhs = makeinst(b);

    const bool result = dis_inst_equal(&lhs, &rhs);

    ct_asserttrue(result);
}

static void inst_eq_same_object(void *ctx)
{
    const uint8_t a[] = {0xad, 0x44, 0x80};
    const struct dis_instruction lhs = makeinst(a);

    const bool result = dis_inst_equal(&lhs, &lhs);

    ct_asserttrue(result);
}

//
// Disassemble Instruction
//

static void inst_does_nothing_if_no_bytes(void *ctx)
{
    const uint16_t a = 0x1234;
    const struct dis_instruction inst = {0};
    char buf[DIS_INST_SIZE] = {'\0'};

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_implied(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xea};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: EA        NOP";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_immediate(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa9, 0x34};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: A9 34     LDA #$34";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_zeropage(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa5, 0x34};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: A5 34     LDA $34";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_zeropage_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb5, 0x34};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: B5 34     LDA $34,X";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_zeropage_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb6, 0x34};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: B6 34     LDX $34,Y";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_indirect_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa1, 0x34};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: A1 34     LDA ($34,X)";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_indirect_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb1, 0x34};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: B1 34     LDA ($34),Y";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_absolute(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xad, 0x34, 0x6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: AD 34 06  LDA $0634";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_absolute_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xbd, 0x34, 0x6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: BD 34 06  LDA $0634,X";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_absolute_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb9, 0x34, 0x6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: B9 34 06  LDA $0634,Y";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_jmp_absolute(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x4c, 0x34, 0x6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 4C 34 06  JMP $0634";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_jmp_indirect(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x6c, 0x34, 0x6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 6C 34 06  JMP ($0634)";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_branch_positive(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x90, 0xa};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 90 0A     BCC +10";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_branch_negative(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x90, 0xf6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 90 F6     BCC -10";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_branch_zero(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x90, 0x0};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 90 00     BCC +0";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_push(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x48};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 48        PHA";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_pull(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x68};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 68        PLA";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_jsr(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x20, 0x34, 0x6};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 20 34 06  JSR $0634";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_rts(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x60};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 60        RTS";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_brk(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x0};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 00        BRK";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void inst_disassembles_unofficial(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x02};
    const struct dis_instruction inst = makeinst(bytes);
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, &inst, buf);

    const char *const exp = "1234: 02       *JAM";
    ct_assertequal((int)strlen(exp), length);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// Disassemble Peek
//

static void peek_immediate(void *ctx)
{
    // NOTE: LDA #$10
    uint8_t mem[] = {0xa9, 0x10};
    struct mos6502 cpu;
    char buf[DIS_PEEK_SIZE];
    setup_cpu(&cpu, mem, NULL);
    struct console_state snapshot;
    cpu.a = 0x10;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "";
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
    struct console_state snapshot;
    cpu.a = 0x10;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "= 20";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 2;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "@ 05 = 30";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 2;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "@ 04 > 0102 = 40";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.y = 5;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "> 0102 @ 0107 = 60";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 0xa;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "@ 010C = 70";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = true;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "@ 0007";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.p.z = false;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "@ 0007";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.x = 0xa;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "> 0205";
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
    struct console_state snapshot;
    cpu.a = 0x10;
    cpu.irq = NIS_COMMITTED;
    cpu_snapshot(&cpu, &snapshot);
    snapshot.debugger.resvector_override = -1;
    snapshot.mem.vectors[4] = 0xbb;
    snapshot.mem.vectors[5] = 0xaa;

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "(IRQ) > AABB";
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
    struct console_state snapshot;
    cpu.a = 0x10;
    cpu.res = NIS_COMMITTED;
    cpu_snapshot(&cpu, &snapshot);
    snapshot.debugger.resvector_override = 0xccdd;
    snapshot.mem.vectors[2] = 0xbb;
    snapshot.mem.vectors[3] = 0xaa;

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "(RES) > !CCDD";
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
    struct console_state snapshot;
    cpu.a = 0x10;
    cpu.nmi = NIS_COMMITTED;
    cpu_snapshot(&cpu, &snapshot);
    snapshot.debugger.resvector_override = 0xccdd;
    snapshot.mem.vectors[0] = 0xff;
    snapshot.mem.vectors[1] = 0xee;
    snapshot.mem.vectors[2] = 0xbb;
    snapshot.mem.vectors[3] = 0xaa;

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "(NMI) > EEFF";
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
    struct console_state snapshot;
    setup_cpu(&cpu, mem, NULL);
    cpu.a = 0x10;
    cpu.y = 5;
    cpu_snapshot(&cpu, &snapshot);

    const int written = dis_peek(0x0, &cpu, &snapshot, buf);

    const char *const exp = "> 4002 @ 4007 = FLT";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
    ct_assertfalse(cpu.detached);
}

//
// Disassemble Datapath
//

static void datapath_end_of_rom(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xea},
            .prglength = 1,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_unexpected_end_of_rom(void *ctx)
{
    // NOTE: LDA imm with missing 2nd byte
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xa9},
            .prglength = 1,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE] = {'\0'};

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "";
    ct_assertequal(DIS_ERR_EOF, written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_implied_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_implied_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_implied_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_immediate_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xa9, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA imm";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_immediate_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xa9, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA #$43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_immediate_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xa9, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA #$43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xa5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA zp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xa5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xa5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_x_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xb5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA zp,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_x_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xb5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_x_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xb5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_y_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xb6, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDX zp,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_y_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xb6, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDX $43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_zeropage_y_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xb6, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDX $43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_x_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xa1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA (zp,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_x_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xa1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_x_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xa1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_y_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xb1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA (zp),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_y_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xb1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_indirect_y_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xb1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_two(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 3,
        },
        .mem = {
            .currprg = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA abs,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $??43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_two(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_x_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 3,
        },
        .mem = {
            .currprg = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA abs,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $??43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_two(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_absolute_y_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 3,
        },
        .mem = {
            .currprg = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_two(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_absolute_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 3,
        },
        .mem = {
            .currprg = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP (abs)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP ($??43)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_two(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP ($2143)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jmp_indirect_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 3,
        },
        .mem = {
            .currprg = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP ($2143)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_bch_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x90, 0x2, 0xff, 0xff, 0xff},
            .prglength = 5,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BCC rel";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_bch_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x90, 0x2, 0xff, 0xff, 0xff},
            .prglength = 5,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BCC +2";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_bch_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x90, 0x2, 0xff, 0xff, 0xff},
            .prglength = 5,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BCC +2";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_push_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x48, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "PHA imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_push_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x48, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "PHA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_push_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x48, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "PHA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_pull_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x68, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "PLA imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_pull_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x68, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "PLA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_pull_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x68, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "PLA ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JSR abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JSR $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_two(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JSR $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_jsr_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 3,
        },
        .mem = {
            .currprg = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JSR $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rts_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x60, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "RTS imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rts_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x60, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "RTS ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_rts_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x60, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "RTS ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
        },
        .mem = {
            .currprg = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
        },
        .mem = {
            .currprg = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
        },
        .mem = {
            .currprg = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_brk_cycle_six(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 6,
        },
        .mem = {
            .currprg = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.currprg[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
            .irq = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
            .irq = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK (IRQ)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
            .irq = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK (IRQ)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_irq_cycle_six(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 6,
            .irq = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
            .nmi = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
            .nmi = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK (NMI)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
            .nmi = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK (NMI)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_nmi_cycle_six(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 6,
            .nmi = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_res_cycle_zero(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 0,
            .res = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_res_cycle_one(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 1,
            .res = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK (RES)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_res_cycle_n(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 2,
            .res = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK (RES)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void datapath_res_cycle_six(void *ctx)
{
    struct console_state sn = {
        .datapath = {
            .exec_cycle = 6,
            .res = NIS_COMMITTED,
        },
        .mem = {
            .currprg = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = BrkOpcode;
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// Test List
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

        ct_maketest(datapath_res_cycle_zero),
        ct_maketest(datapath_res_cycle_one),
        ct_maketest(datapath_res_cycle_n),
        ct_maketest(datapath_res_cycle_six),
    };

    return ct_makesuite(tests);
}

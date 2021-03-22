//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "asm.h"
#include "ciny.h"
#include "emu/snapshot.h"

#include <stdint.h>
#include <string.h>

//
// dis_errstr
//

static void dis_errstr_returns_known_err(void *ctx)
{
    const char *const err = dis_errstr(ASM_FMT_FAIL);

    ct_assertequalstr("OUTPUT FAIL", err);
}

static void dis_errstr_returns_unknown_err(void *ctx)
{
    const char *const err = dis_errstr(10);

    ct_assertequalstr("UNKNOWN ERR", err);
}

//
// dis_instaddr
//

static void dis_instaddr_one_byte_cycle_zero(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .opcode = 0xea,
            .program_counter = 0x8001,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_one_byte_cycle_one(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .opcode = 0xea,
            .program_counter = 0x8001,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_one_byte_cycle_n(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .opcode = 0xea,
            .program_counter = 0x8001,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_two_byte_cycle_zero(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .opcode = 0xa9,
            .program_counter = 0x8001,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_two_byte_cycle_one(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .opcode = 0xa9,
            .program_counter = 0x8002,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_two_byte_cycle_n(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .opcode = 0xa9,
            .program_counter = 0x8002,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_three_byte_cycle_zero(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .opcode = 0xad,
            .program_counter = 0x8001,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_three_byte_cycle_one(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .opcode = 0xad,
            .program_counter = 0x8002,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_three_byte_cycle_two(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .opcode = 0xad,
            .program_counter = 0x8003,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_three_byte_cycle_n(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 3,
            .opcode = 0xad,
            .program_counter = 0x8003,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8000u, result);
}

static void dis_instaddr_jmp_complete_no_offset(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .instdone = true,
            .opcode = 0x4c,
            .program_counter = 0x8003,
        },
    };

    const uint16_t result = dis_instaddr(&sn);

    ct_assertequal(0x8003u, result);
}

//
// dis_inst
//

static void dis_inst_does_nothing_if_no_bytes(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[1];
    char buf[DIS_INST_SIZE] = {'\0'};

    const int length = dis_inst(a, bytes, 0, buf);

    ct_assertequal(0, length);
    ct_assertequalstr("", buf);
}

static void dis_inst_disassembles_implied(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xea};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(1, length);
    ct_assertequalstr("$1234: EA          NOP", buf);
}

static void dis_inst_disassembles_immediate(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa9, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(2, length);
    ct_assertequalstr("$1234: A9 34       LDA #$34", buf);
}

static void dis_inst_disassembles_zeropage(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa5, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(2, length);
    ct_assertequalstr("$1234: A5 34       LDA $34", buf);
}

static void dis_inst_disassembles_zeropage_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb5, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(2, length);
    ct_assertequalstr("$1234: B5 34       LDA $34,X", buf);
}

static void dis_inst_disassembles_zeropage_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb6, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(2, length);
    ct_assertequalstr("$1234: B6 34       LDX $34,Y", buf);
}

static void dis_inst_disassembles_indirect_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa1, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(2, length);
    ct_assertequalstr("$1234: A1 34       LDA ($34,X)", buf);
}

static void dis_inst_disassembles_indirect_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb1, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(2, length);
    ct_assertequalstr("$1234: B1 34       LDA ($34),Y", buf);
}

static void dis_inst_disassembles_absolute(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xad, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(3, length);
    ct_assertequalstr("$1234: AD 34 06    LDA $0634", buf);
}

static void dis_inst_disassembles_absolute_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xbd, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(3, length);
    ct_assertequalstr("$1234: BD 34 06    LDA $0634,X", buf);
}

static void dis_inst_disassembles_absolute_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb9, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(3, length);
    ct_assertequalstr("$1234: B9 34 06    LDA $0634,Y", buf);
}

static void dis_inst_disassembles_jmp_absolute(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x4c, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(3, length);
    ct_assertequalstr("$1234: 4C 34 06    JMP $0634", buf);
}

static void dis_inst_disassembles_jmp_indirect(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x6c, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes, buf);

    ct_assertequal(3, length);
    ct_assertequalstr("$1234: 6C 34 06    JMP ($0634)", buf);
}

//
// dis_datapath
//

static void dis_datapath_addr_too_low(void *ctx)
{
    const uint8_t rom[] = {0xea, 0xff};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x10,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE] = {'\0'};

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "";
    ct_assertequal(ASM_EOF, written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_offset_overflow(void *ctx)
{
    const uint8_t rom[] = {0xad, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0xfffe,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE] = {'\0'};

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "";
    ct_assertequal(ASM_EOF, written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_implied_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xea, 0xff};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP imp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_implied_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xea, 0xff};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_implied_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xea, 0xff};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "NOP ";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_immediate_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xa9, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA imm";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_immediate_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xa9, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA #$43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_immediate_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xa9, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA #$43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xa5, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA zp";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xa5, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xa5, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_x_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xb5, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA zp,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_x_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xb5, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_x_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xb5, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_y_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xb6, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDX zp,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_y_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xb6, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDX $43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_zeropage_y_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xb6, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDX $43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_indirect_x_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xa1, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA (zp,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_indirect_x_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xa1, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_indirect_x_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xa1, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43,X)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_indirect_y_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xb1, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA (zp),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_indirect_y_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xb1, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_indirect_y_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xb1, 0x43};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA ($43),Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xad, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xad, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_cycle_two(void *ctx)
{
    const uint8_t rom[] = {0xad, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xad, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 3,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_x_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xbd, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA abs,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_x_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xbd, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $??43,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_x_cycle_two(void *ctx)
{
    const uint8_t rom[] = {0xbd, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_x_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xbd, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 3,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,X";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_y_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0xb9, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA abs,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_y_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0xb9, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $??43,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_y_cycle_two(void *ctx)
{
    const uint8_t rom[] = {0xb9, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 2,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_absolute_y_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0xb9, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 3,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "LDA $2143,Y";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_absolute_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0x4c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP abs";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_absolute_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0x4c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .addra_latch = rom[1],
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP $??43";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_absolute_cycle_two(void *ctx)
{
    const uint8_t rom[] = {0x4c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .addra_latch = rom[1],
            .databus = rom[2],
            .exec_cycle = 2,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_absolute_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0x4c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .addra_latch = rom[1],
            .databus = rom[2],
            .exec_cycle = 3,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP $2143";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_indirect_cycle_zero(void *ctx)
{
    const uint8_t rom[] = {0x6c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .program_counter = 0x8001,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP (abs)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_indirect_cycle_one(void *ctx)
{
    const uint8_t rom[] = {0x6c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .addrc_latch = rom[1],
            .exec_cycle = 1,
            .program_counter = 0x8002,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP ($??43)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_indirect_cycle_two(void *ctx)
{
    const uint8_t rom[] = {0x6c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .addrc_latch = rom[1],
            .addrb_latch = rom[2],
            .exec_cycle = 2,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP ($2143)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

static void dis_datapath_jmp_indirect_cycle_n(void *ctx)
{
    const uint8_t rom[] = {0x6c, 0x43, 0x21};
    const struct console_state sn = {
        .cpu = {
            .addrc_latch = rom[1] + 1,
            .addrb_latch = rom[2],
            .exec_cycle = 3,
            .program_counter = 0x8003,
            .opcode = rom[0],
        },
        .rom = rom,
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "JMP ($2143)";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// Test List
//

struct ct_testsuite dis_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(dis_errstr_returns_known_err),
        ct_maketest(dis_errstr_returns_unknown_err),

        ct_maketest(dis_instaddr_one_byte_cycle_zero),
        ct_maketest(dis_instaddr_one_byte_cycle_one),
        ct_maketest(dis_instaddr_one_byte_cycle_n),
        ct_maketest(dis_instaddr_two_byte_cycle_zero),
        ct_maketest(dis_instaddr_two_byte_cycle_one),
        ct_maketest(dis_instaddr_two_byte_cycle_n),
        ct_maketest(dis_instaddr_three_byte_cycle_zero),
        ct_maketest(dis_instaddr_three_byte_cycle_one),
        ct_maketest(dis_instaddr_three_byte_cycle_two),
        ct_maketest(dis_instaddr_three_byte_cycle_n),
        ct_maketest(dis_instaddr_jmp_complete_no_offset),

        ct_maketest(dis_inst_does_nothing_if_no_bytes),
        ct_maketest(dis_inst_disassembles_implied),
        ct_maketest(dis_inst_disassembles_immediate),
        ct_maketest(dis_inst_disassembles_zeropage),
        ct_maketest(dis_inst_disassembles_zeropage_x),
        ct_maketest(dis_inst_disassembles_zeropage_y),
        ct_maketest(dis_inst_disassembles_indirect_x),
        ct_maketest(dis_inst_disassembles_indirect_y),
        ct_maketest(dis_inst_disassembles_absolute),
        ct_maketest(dis_inst_disassembles_absolute_x),
        ct_maketest(dis_inst_disassembles_absolute_y),
        ct_maketest(dis_inst_disassembles_jmp_absolute),
        ct_maketest(dis_inst_disassembles_jmp_indirect),

        ct_maketest(dis_datapath_addr_too_low),
        ct_maketest(dis_datapath_offset_overflow),

        ct_maketest(dis_datapath_implied_cycle_zero),
        ct_maketest(dis_datapath_implied_cycle_one),
        ct_maketest(dis_datapath_implied_cycle_n),

        ct_maketest(dis_datapath_immediate_cycle_zero),
        ct_maketest(dis_datapath_immediate_cycle_one),
        ct_maketest(dis_datapath_immediate_cycle_n),

        ct_maketest(dis_datapath_zeropage_cycle_zero),
        ct_maketest(dis_datapath_zeropage_cycle_one),
        ct_maketest(dis_datapath_zeropage_cycle_n),

        ct_maketest(dis_datapath_zeropage_x_cycle_zero),
        ct_maketest(dis_datapath_zeropage_x_cycle_one),
        ct_maketest(dis_datapath_zeropage_x_cycle_n),

        ct_maketest(dis_datapath_zeropage_y_cycle_zero),
        ct_maketest(dis_datapath_zeropage_y_cycle_one),
        ct_maketest(dis_datapath_zeropage_y_cycle_n),

        ct_maketest(dis_datapath_indirect_x_cycle_zero),
        ct_maketest(dis_datapath_indirect_x_cycle_one),
        ct_maketest(dis_datapath_indirect_x_cycle_n),

        ct_maketest(dis_datapath_indirect_y_cycle_zero),
        ct_maketest(dis_datapath_indirect_y_cycle_one),
        ct_maketest(dis_datapath_indirect_y_cycle_n),

        ct_maketest(dis_datapath_absolute_cycle_zero),
        ct_maketest(dis_datapath_absolute_cycle_one),
        ct_maketest(dis_datapath_absolute_cycle_two),
        ct_maketest(dis_datapath_absolute_cycle_n),

        ct_maketest(dis_datapath_absolute_x_cycle_zero),
        ct_maketest(dis_datapath_absolute_x_cycle_one),
        ct_maketest(dis_datapath_absolute_x_cycle_two),
        ct_maketest(dis_datapath_absolute_x_cycle_n),

        ct_maketest(dis_datapath_absolute_y_cycle_zero),
        ct_maketest(dis_datapath_absolute_y_cycle_one),
        ct_maketest(dis_datapath_absolute_y_cycle_two),
        ct_maketest(dis_datapath_absolute_y_cycle_n),

        ct_maketest(dis_datapath_jmp_absolute_cycle_zero),
        ct_maketest(dis_datapath_jmp_absolute_cycle_one),
        ct_maketest(dis_datapath_jmp_absolute_cycle_two),
        ct_maketest(dis_datapath_jmp_absolute_cycle_n),

        ct_maketest(dis_datapath_jmp_indirect_cycle_zero),
        ct_maketest(dis_datapath_jmp_indirect_cycle_one),
        ct_maketest(dis_datapath_jmp_indirect_cycle_two),
        ct_maketest(dis_datapath_jmp_indirect_cycle_n),
    };

    return ct_makesuite(tests);
}

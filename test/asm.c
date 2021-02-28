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

static void dis_inst_disassembles_onebyte(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[1] = {0xea};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, 1, buf);

    ct_assertequal(1, length);
    ct_assertequalstr("$1234: EA          NOP", buf);
}

//
// dis_datapath
//

static void dis_datapath_implied_cyclezero(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 0,
            .opcode = 0xea,
        },
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    ct_assertequal(7, written);
    ct_assertequalstr("NOP imp", buf);
}

static void dis_datapath_implied_cycleone(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .databus = 0xff,
            .exec_cycle = 1,
            .opcode = 0xea,
        },
    };
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    ct_assertequal(4, written);
    ct_assertequalstr("NOP ", buf);
}

static void dis_datapath_implied_cyclen(void *ctx)
{
    const struct console_state sn = {
        .cpu = {
            .exec_cycle = 3,
        },
    };
    char buf[DIS_DATAP_SIZE] = {'\0'};

    const int written = dis_datapath(&sn, buf);

    ct_assertequal(0, written);
    ct_assertequalstr("", buf);
}

//
// Test List
//

struct ct_testsuite asm_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(dis_errstr_returns_known_err),
        ct_maketest(dis_errstr_returns_unknown_err),

        ct_maketest(dis_inst_does_nothing_if_no_bytes),
        ct_maketest(dis_inst_disassembles_onebyte),

        ct_maketest(dis_datapath_implied_cyclezero),
        ct_maketest(dis_datapath_implied_cycleone),
        ct_maketest(dis_datapath_implied_cyclen),
    };

    return ct_makesuite(tests);
}

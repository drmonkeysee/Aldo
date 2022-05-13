//
//  asm.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "ciny.h"
#include "cpu.h"
#include "cpuhelp.h"
#include "dis.h"
#include "snapshot.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
// Disassemble Instruction
//

static void inst_does_nothing_if_no_bytes(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[1];
    char buf[DIS_INST_SIZE] = {'\0'};

    const int length = dis_inst(a, bytes, 0, buf);

    ct_assertequal(0, length);
    ct_assertequalstr("", buf);
}

static void inst_eof(void *ctx)
{
    const uint16_t a = 0x1234;
    // NOTE: LDA abs with missing 3rd byte
    const uint8_t bytes[] = {0xad, 0x43};
    char buf[DIS_INST_SIZE] = {'\0'};

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(DIS_ERR_EOF, length);
    ct_assertequalstr("", buf);
}

static void inst_disassembles_implied(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xea};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(1, length);
    ct_assertequalstr("1234: EA        NOP", buf);
}

static void inst_disassembles_immediate(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa9, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: A9 34     LDA #$34", buf);
}

static void inst_disassembles_zeropage(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa5, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: A5 34     LDA $34", buf);
}

static void inst_disassembles_zeropage_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb5, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: B5 34     LDA $34,X", buf);
}

static void inst_disassembles_zeropage_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb6, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: B6 34     LDX $34,Y", buf);
}

static void inst_disassembles_indirect_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xa1, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: A1 34     LDA ($34,X)", buf);
}

static void inst_disassembles_indirect_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb1, 0x34};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: B1 34     LDA ($34),Y", buf);
}

static void inst_disassembles_absolute(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xad, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(3, length);
    ct_assertequalstr("1234: AD 34 06  LDA $0634", buf);
}

static void inst_disassembles_absolute_x(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xbd, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(3, length);
    ct_assertequalstr("1234: BD 34 06  LDA $0634,X", buf);
}

static void inst_disassembles_absolute_y(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0xb9, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(3, length);
    ct_assertequalstr("1234: B9 34 06  LDA $0634,Y", buf);
}

static void inst_disassembles_jmp_absolute(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x4c, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(3, length);
    ct_assertequalstr("1234: 4C 34 06  JMP $0634", buf);
}

static void inst_disassembles_jmp_indirect(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x6c, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(3, length);
    ct_assertequalstr("1234: 6C 34 06  JMP ($0634)", buf);
}

static void inst_disassembles_branch_positive(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x90, 0xa};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: 90 0A     BCC +10", buf);
}

static void inst_disassembles_branch_negative(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x90, 0xf6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: 90 F6     BCC -10", buf);
}

static void inst_disassembles_branch_zero(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x90, 0x0};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(2, length);
    ct_assertequalstr("1234: 90 00     BCC +0", buf);
}

static void inst_disassembles_push(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x48};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(1, length);
    ct_assertequalstr("1234: 48        PHA", buf);
}

static void inst_disassembles_pull(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x68};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(1, length);
    ct_assertequalstr("1234: 68        PLA", buf);
}

static void inst_disassembles_jsr(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x20, 0x34, 0x6};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(3, length);
    ct_assertequalstr("1234: 20 34 06  JSR $0634", buf);
}

static void inst_disassembles_rts(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x60};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(1, length);
    ct_assertequalstr("1234: 60        RTS", buf);
}

static void inst_disassembles_brk(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x0};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(1, length);
    ct_assertequalstr("1234: 00        BRK", buf);
}

static void inst_disassembles_unofficial(void *ctx)
{
    const uint16_t a = 0x1234;
    const uint8_t bytes[] = {0x02};
    char buf[DIS_INST_SIZE];

    const int length = dis_inst(a, bytes, sizeof bytes / sizeof bytes[0], buf);

    ct_assertequal(1, length);
    ct_assertequalstr("1234: 02       *JAM", buf);
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
            .prgview = {0xea},
            .prglength = 1,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa9},
            .prglength = 1,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xea, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa9, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa9, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa9, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb5, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb6, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb6, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb6, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xa1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb1, 0x43},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xad, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xbd, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0xb9, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x4c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x6c, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x90, 0x2, 0xff, 0xff, 0xff},
            .prglength = 5,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x90, 0x2, 0xff, 0xff, 0xff},
            .prglength = 5,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x90, 0x2, 0xff, 0xff, 0xff},
            .prglength = 5,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x48, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x48, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x48, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x68, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x68, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x68, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x20, 0x43, 0x21},
            .prglength = 3,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x60, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x60, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x60, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
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
            .prgview = {0x0, 0xff},
            .prglength = 2,
        },
    };
    sn.datapath.opcode = sn.mem.prgview[0];
    char buf[DIS_DATAP_SIZE];

    const int written = dis_datapath(&sn, buf);

    const char *const exp = "BRK CLR";
    ct_assertequal((int)strlen(exp), written);
    ct_assertequalstrn(exp, buf, sizeof exp);
}

//
// Parse Instructions
//

static void parse_inst_empty_bankview(void *ctx)
{
    struct bankview bv = {.size = 0};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 0, &inst);

    ct_assertequal(0, result);
}

static void parse_inst_at_start(void *ctx)
{
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct bankview bv = {.mem = mem, .size = sizeof mem / sizeof *mem};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 0, &inst);

    ct_assertequal(1, result);
    ct_assertequal(0xeau, inst.bytes[0]);
    ct_assertequal(1u, inst.length);
    ct_assertequal(IN_NOP, (int)inst.decode.instruction);
    ct_assertequal(AM_IMP, (int)inst.decode.mode);
    ct_assertfalse(inst.decode.unofficial);
}

static void parse_inst_in_middle(void *ctx)
{
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct bankview bv = {.mem = mem, .size = sizeof mem / sizeof *mem};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 3, &inst);

    ct_assertequal(3, result);
    ct_assertequal(0x4cu, inst.bytes[0]);
    ct_assertequal(0x34u, inst.bytes[1]);
    ct_assertequal(0x6u, inst.bytes[2]);
    ct_assertequal(3u, inst.length);
    ct_assertequal(IN_JMP, (int)inst.decode.instruction);
    ct_assertequal(AM_JABS, (int)inst.decode.mode);
    ct_assertfalse(inst.decode.unofficial);
}

static void parse_inst_unofficial(void *ctx)
{
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct bankview bv = {.mem = mem, .size = sizeof mem / sizeof *mem};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 2, &inst);

    ct_assertequal(2, result);
    ct_assertequal(0x34u, inst.bytes[0]);
    ct_assertequal(0x4cu, inst.bytes[1]);
    ct_assertequal(2u, inst.length);
    ct_assertequal(IN_NOP, (int)inst.decode.instruction);
    ct_assertequal(AM_ZPX, (int)inst.decode.mode);
    ct_asserttrue(inst.decode.unofficial);
}

static void parse_inst_eof(void *ctx)
{
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct bankview bv = {.mem = mem, .size = sizeof mem / sizeof *mem};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 5, &inst);

    ct_assertequal(DIS_ERR_EOF, result);
}

static void parse_inst_out_of_bounds(void *ctx)
{
    const uint8_t mem[] = {0xea, 0xa5, 0x34, 0x4c, 0x34, 0x6};
    struct bankview bv = {.mem = mem, .size = sizeof mem / sizeof *mem};
    struct dis_instruction inst;

    const int result = dis_parse_inst(&bv, 10, &inst);

    ct_assertequal(0, result);
}

//
// Test List
//

struct ct_testsuite dis_tests(void)
{
    static const struct ct_testcase tests[] = {
        ct_maketest(errstr_returns_known_err),
        ct_maketest(errstr_returns_unknown_err),

        ct_maketest(inst_does_nothing_if_no_bytes),
        ct_maketest(inst_eof),
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

        ct_maketest(parse_inst_empty_bankview),
        ct_maketest(parse_inst_at_start),
        ct_maketest(parse_inst_in_middle),
        ct_maketest(parse_inst_unofficial),
        ct_maketest(parse_inst_eof),
        ct_maketest(parse_inst_out_of_bounds),
    };

    return ct_makesuite(tests);
}

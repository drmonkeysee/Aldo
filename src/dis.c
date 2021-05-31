//
//  dis.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/21.
//

#include "dis.h"

#include "emu/bytes.h"
#include "emu/decode.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *restrict const Mnemonics[] = {
#define X(s, ...) #s,
    DEC_INST_X
#undef X
};

static const int InstLens[] = {
#define X(s, b, ...) b,
    DEC_ADDRMODE_X
#undef X
};

#define STRTABLE(s) s##_StringTable

#define X(s, b, ...) static const char *restrict const STRTABLE(s)[] = { \
    __VA_ARGS__ \
};
    DEC_ADDRMODE_X
#undef X

static const char *restrict const *const StringTables[] = {
#define X(s, b, ...) STRTABLE(s),
    DEC_ADDRMODE_X
#undef X
};

static const char *interrupt_display(const struct console_state *snapshot)
{
    if (snapshot->datapath.exec_cycle == 6) return "CLR";
    if (snapshot->datapath.res == NIS_COMMITTED) return "(RES)";
    if (snapshot->datapath.nmi == NIS_COMMITTED) return "(NMI)";
    if (snapshot->datapath.irq == NIS_COMMITTED) return "(IRQ)";
    return "";
}

static int print_raw(uint16_t addr, const uint8_t *restrict bytes, int instlen,
                     char dis[restrict static DIS_INST_SIZE])
{
    int total, count;
    total = count = sprintf(dis, "$%04X: ", addr);
    if (count < 0) return DIS_ERR_FMT_FAIL;

    for (int i = 0; i < instlen; ++i) {
        count = sprintf(dis + total, "%02X ", bytes[i]);
        if (count < 0) return DIS_ERR_FMT_FAIL;
        total += count;
    }

    return total;
}

static int print_mnemonic(const struct decoded *dec,
                          const uint8_t *restrict bytes, int instlen,
                          char dis[restrict])
{
    int total, count;
    total = count = sprintf(dis, "%s ", Mnemonics[dec->instruction]);
    if (count < 0) return DIS_ERR_FMT_FAIL;

    const char *restrict const *const strtable = StringTables[dec->mode];
    switch (instlen) {
    case 1:
        // NOTE: nothing else to print, trim the trailing space
        dis[--count] = '\0';
        break;
    case 2:
        count = sprintf(dis + total, strtable[instlen - 1], bytes[1]);
        break;
    case 3:
        count = sprintf(dis + total, strtable[instlen - 1], batowr(bytes + 1));
        break;
    default:
        return DIS_ERR_INV_ADDRMD;
    }
    if (count < 0) return DIS_ERR_FMT_FAIL;
    return total + count;
}

enum duplicate_state {
    DUP_NONE,
    DUP_FIRST,
    DUP_TRUNCATE,
    DUP_SKIP,
    DUP_VERBOSE,
};

struct repeat_condition {
    uint32_t prev_bytes;
    enum duplicate_state state;
};

static void print_prg_line(const char *restrict dis, uint32_t curr_bytes,
                           size_t total, size_t banksize,
                           struct repeat_condition *repeat)
{
    if (repeat->state == DUP_VERBOSE) {
        puts(dis);
        return;
    }

    if (curr_bytes == repeat->prev_bytes) {
        switch (repeat->state) {
        case DUP_NONE:
            repeat->state = DUP_FIRST;
            puts(dis);
            break;
        case DUP_FIRST:
            repeat->state = DUP_TRUNCATE;
            // NOTE: if this is the last instruction in the PRG bank
            // skip printing the truncation indicator.
            if (total < banksize) {
                puts("*");
            }
            break;
        default:
            repeat->state = DUP_SKIP;
            break;
        }
    } else {
        puts(dis);
        repeat->prev_bytes = curr_bytes;
        repeat->state = DUP_NONE;
    }
}

static int print_bank(const struct bankview *bv, bool verbose)
{
    printf("Bank %zu (%zuKB)\n", bv->bank, bv->size >> BITWIDTH_1KB);
    puts("--------");

    int bytes_read = 0;
    struct repeat_condition repeat = {
        .state = verbose ? DUP_VERBOSE : DUP_NONE,
    };
    char dis[DIS_INST_SIZE];
    for (size_t total = 0; total < bv->size; total += bytes_read) {
        // TODO: how to pick correct start address?
        const uint16_t addr = MEMBLOCK_32KB + total;
        const uint8_t *const prgoffset = bv->mem + total;
        bytes_read = dis_inst(addr, prgoffset, bv->size - total, dis);
        if (bytes_read == 0) break;
        if (bytes_read < 0) {
            fprintf(stderr, "$%04X: Dis err (%d): %s\n", addr, bytes_read,
                    dis_errstr(bytes_read));
            return bytes_read;
        }
        // NOTE: convert current instruction bytes into easily comparable
        // value to check for repeats.
        uint32_t curr_bytes = 0;
        for (int i = 0; i < bytes_read; ++i) {
            curr_bytes |= prgoffset[i] << (8 * i);
        }
        print_prg_line(dis, curr_bytes, total, bv->size, &repeat);
    }

    // NOTE: always print the last line regardless of duplicate state
    // (if it hasn't already been printed).
    if (repeat.state == DUP_TRUNCATE || repeat.state == DUP_SKIP) {
        puts(dis);
    }

    return 0;
}

//
// Public Interface
//

const char *dis_errstr(int err)
{
    switch (err) {
#define X(s, v, e) case s: return e;
        DIS_ERRCODE_X
#undef X
    default:
        return "UNKNOWN ERR";
    }
}

int dis_inst(uint16_t addr, const uint8_t *restrict bytes, ptrdiff_t bytesleft,
             char dis[restrict static DIS_INST_SIZE])
{
    assert(bytes != NULL);
    assert(bytesleft >= 0);
    assert(dis != NULL);

    if (bytesleft == 0) return 0;

    const struct decoded dec = Decode[*bytes];
    const int instlen = InstLens[dec.mode];
    if (bytesleft < instlen) return DIS_ERR_EOF;

    int count;
    unsigned int total;
    total = count = print_raw(addr, bytes, instlen, dis);
    if (count < 0) return count;

    // NOTE: padding between raw bytes and disassembled instruction
    count = sprintf(dis + total, "%*s", (4 - instlen) * 3, "");
    if (count < 0) return DIS_ERR_FMT_FAIL;
    total += count;

    count = print_mnemonic(&dec, bytes, instlen, dis + total);
    if (count < 0) return count;
    total += count;

    assert(total < DIS_INST_SIZE);
    return instlen;
}

int dis_datapath(const struct console_state *snapshot,
                 char dis[restrict static DIS_DATAP_SIZE])
{
    assert(snapshot != NULL);
    assert(dis != NULL);

    const struct decoded dec = Decode[snapshot->datapath.opcode];
    const int instlen = InstLens[dec.mode];
    if ((size_t)instlen > snapshot->mem.prglength) return DIS_ERR_EOF;

    int count;
    unsigned int total;
    total = count = sprintf(dis, "%s ", Mnemonics[dec.instruction]);
    if (count < 0) return DIS_ERR_FMT_FAIL;

    const int max_offset = 1 + (instlen / 3),
              displayidx = snapshot->datapath.exec_cycle < max_offset
                           ? snapshot->datapath.exec_cycle
                           : max_offset;
    const char *const displaystr = StringTables[dec.mode][displayidx];
    switch (displayidx) {
    case 0:
        count = sprintf(dis + total, "%s", displaystr);
        break;
    case 1:
        count = dec.instruction == IN_BRK
                ? sprintf(dis + total, displaystr, interrupt_display(snapshot))
                : sprintf(dis + total, displaystr,
                          strlen(displaystr) > 0
                            ? snapshot->mem.prgview[1]
                            : 0);
        break;
    default:
        count = sprintf(dis + total, displaystr,
                        strlen(displaystr) > 0
                            ? batowr(snapshot->mem.prgview + 1)
                            : 0);
        break;
    }
    if (count < 0) return DIS_ERR_FMT_FAIL;
    total += count;

    assert(total < DIS_DATAP_SIZE);
    return total;
}

int dis_cart(cart *cart, const struct control *appstate)
{
    assert(cart != NULL);
    assert(appstate != NULL);

    puts(appstate->cartfile);
    cart_write_dis_header(cart, stdout);

    for (struct bankview bv = cart_prgbank(cart, 0);
         bv.mem;
         bv = cart_prgbank(cart, bv.bank + 1)) {
        puts("");
        const int err = print_bank(&bv, appstate->verbose);
        if (err < 0) return err;
    }
    return 0;
}

//
//  argparse.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/23/21.
//

#include "argparse.h"

#include "bytes.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char
    *const restrict Version = "0.4.1", // TODO: autogenerate this

    *const restrict BatchLong = "--batch",
    *const restrict BcdLong = "--bcd",
    *const restrict ChrDecodeLong = "--chr-decode",
    *const restrict ChrScaleLong = "--chr-scale",
    *const restrict DisassembleLong = "--disassemble",
    *const restrict HaltLong = "--halt",
    *const restrict HelpLong = "--help",
    *const restrict InfoLong = "--info",
    *const restrict ResVectorLong = "--reset-vector",
    *const restrict TraceLong = "--trace",
    *const restrict VersionLong = "--version",
    *const restrict ZeroRamLong = "--zero-ram";

static const char
    BatchShort = 'b',
    BcdShort = 'D',
    ChrDecodeShort = 'c',
    ChrScaleShort = 's',
    DisassembleShort = 'd',
    HaltShort = 'H',
    HelpShort = 'h',
    InfoShort = 'i',
    ResVectorShort = 'r',
    TraceShort = 't',
    VerboseShort = 'v',
    VersionShort = 'V',
    ZeroRamShort = 'z';

static const int
    MinAddress = 0,
    MaxAddress = ADDRMASK_64KB,
    MinChrScale = 1,
    MaxChrScale = 10;

static void init_control(struct control *appstate)
{
    *appstate = (struct control){
        .chrscale = MinChrScale,
        .resetvector = -1,
    };
}

static bool parse_flag(const char *arg, char shrt, bool exact, const char *lng)
{
    const size_t lnglen = lng ? strlen(lng) : 0;
    return (strlen(arg) > 1
                && exact
                    ? arg[1] == shrt
                    : arg[1] != '-' && strchr(arg, shrt) != NULL)
            || (lnglen > 0 && strncmp(arg, lng, lnglen) == 0
                && (arg[lnglen] == '\0' || arg[lnglen] == '='));
}

#define SETFLAG(f, a, s, l) (f) = (f) || parse_flag(a, s, false, l)

static bool convert_num(const char *restrict arg, int base, long *result)
{
    char *end;
    errno = 0;
    *result = strtol(arg, &end, base);
    if (errno == ERANGE) {
        perror("Number parse failed");
        return false;
    }
    if ((*result == 0 && arg == end) || *end != '\0') {
        return false;
    }
    return true;
}

// NOTE: value of number is undefined if return value is false
static bool parse_number(const char *arg, int *restrict argi, int argc,
                         char *argv[argc+1], int base, long *restrict number)
{
    // NOTE: first try -sN format
    bool result = convert_num(arg + 2, base, number);
    // NOTE: then try --long-name=N format
    if (!result && arg[1] == '-') {
        const char *const opt = strchr(arg, '=');
        if (opt) {
            result = convert_num(opt + 1, base, number);
        }
    }
    // NOTE: finally try parsing next argument
    if (!result) {
        result = ++*argi < argc
                    ? convert_num(argv[*argi], base, number)
                    : false;
    }
    return result;
}

static bool parse_address(const char *arg, int *restrict argi, int argc,
                          char *argv[argc+1], const char *restrict label,
                          int *restrict parsed)
{
    long addr;
    const bool result = parse_number(arg, argi, argc, argv, 16, &addr);
    if (result && MinAddress <= addr && addr <= MaxAddress) {
        *parsed = (int)addr;
    } else {
        fprintf(stderr, "Invalid %s format: expected [0x%X, 0x%X]\n", label,
                MinAddress, MaxAddress);
        return false;
    }
    return true;
}

static bool parse_halt(const char *arg, int *restrict argi, int argc,
                       char *argv[argc+1], struct control *restrict appstate)
{
    const size_t optlen = strlen(HaltLong);
    const char *expr = NULL;
    if (arg[1] == HaltShort && arg[2] != '\0') {
        expr = arg + 2;
    } else if (strncmp(arg, HaltLong, optlen) == 0) {
        const char *const opt = strchr(arg, '=');
        if (opt && opt - arg == (ptrdiff_t)optlen) {
            expr = opt + 1;
        }
    }
    if (!expr && ++*argi < argc) {
        expr = argv[*argi];
    }
    if (expr) {
        struct haltarg **tail;
        for (tail = &appstate->haltlist; *tail; tail = &(*tail)->next);
        *tail = malloc(sizeof **tail);
        **tail = (struct haltarg){.expr = expr};
        return true;
    }
    return false;
}

static bool parse_arg(struct control *restrict appstate, const char *arg,
                      int *restrict argi, int argc, char *argv[argc+1])
{
    if (parse_flag(arg, ChrScaleShort, true, ChrScaleLong)) {
        long scale;
        const bool result = parse_number(arg, argi, argc, argv, 10, &scale);
        if (result && MinChrScale <= scale && scale <= MaxChrScale) {
            appstate->chrscale = (int)scale;
            return true;
        }
        fprintf(stderr, "Invalid scale format: expected [%d, %d]\n",
                MinChrScale, MaxChrScale);
        return false;
    }

    if (parse_flag(arg, ResVectorShort, true, ResVectorLong)) {
        return parse_address(arg, argi, argc, argv, "vector",
                             &appstate->resetvector);
    }

    if (parse_flag(arg, HaltShort, false, HaltLong)) {
        return parse_halt(arg, argi, argc, argv, appstate);
    }

    SETFLAG(appstate->chrdecode, arg, ChrDecodeShort, ChrDecodeLong);
    const size_t chroptlen = strlen(ChrDecodeLong);
    if (strncmp(arg, ChrDecodeLong, chroptlen) == 0) {
        const char *const opt = strchr(arg, '=');
        if (opt && opt - arg == (ptrdiff_t)chroptlen) {
            appstate->chrdecode_prefix = opt + 1;
        }
    }

    SETFLAG(appstate->batch, arg, BatchShort, BatchLong);
    SETFLAG(appstate->bcdsupport, arg, BcdShort, BcdLong);
    SETFLAG(appstate->disassemble, arg, DisassembleShort, DisassembleLong);
    SETFLAG(appstate->help, arg, HelpShort, HelpLong);
    SETFLAG(appstate->info, arg, InfoShort, InfoLong);
    SETFLAG(appstate->tron, arg, TraceShort, TraceLong);
    SETFLAG(appstate->verbose, arg, VerboseShort, NULL);
    SETFLAG(appstate->version, arg, VersionShort, VersionLong);
    SETFLAG(appstate->zeroram, arg, ZeroRamShort, ZeroRamLong);

    return true;
}

//
// Public Interface
//

bool argparse_parse(struct control *restrict appstate, int argc,
                    char *argv[argc+1])
{
    init_control(appstate);
    appstate->me = argc > 0 && strlen(argv[0]) > 0 ? argv[0] : "aldo";
    bool opt_parse = true;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const char *const arg = argv[i];
            if (arg[0] == '-') {
                if (!opt_parse) {
                    continue;
                } else if (arg[1] == '-' && arg[2] == '\0') {
                    opt_parse = false;
                } else if (!parse_arg(appstate, arg, &i, argc, argv)) {
                    argparse_cleanup(appstate);
                    return false;
                }
            } else {
                appstate->cartfile = arg;
            }
        }
    } else {
        appstate->help = true;
    }
    return true;
}

void argparse_usage(const char *me)
{
    puts("---=== Aldo Usage ===---");
    printf("%s [options...] [command] file\n", me);
    puts("\noptions");
    printf("  -%c\t: run program in batch mode (alt %s)\n", BatchShort,
           BatchLong);
    printf("  -%c\t: enable BCD (binary-coded decimal) support (alt %s)\n",
           BcdShort, BcdLong);
    printf("  -%c e\t: halt condition expression (alt %s e),"
           "\n\t  multiple -H options can be specified;"
           "\n\t  see below usage section for syntax\n", HaltShort, HaltLong);
    printf("  -%c x\t: override RESET vector [0x%X, 0x%X]"
           " (alt %s x)\n", ResVectorShort, MinAddress, MaxAddress,
           ResVectorLong);
    printf("  -%c n\t: CHR ROM BMP scaling factor [%d, %d]"
           " (alt %s n)\n", ChrScaleShort, MinChrScale, MaxChrScale,
           ChrScaleLong);
    printf("  -%c\t: turn on trace-logging and ram dumps (alt %s)\n",
           TraceShort, TraceLong);
    printf("  -%c\t: verbose output\n", VerboseShort);
    printf("  -%c\t: zero-out RAM on startup (alt %s)\n", ZeroRamShort,
           ZeroRamLong);
    puts("\ncommands");
    printf("  -%c\t: decode CHR ROM into BMP files (alt %s[=prefix]);"
           "\n\t  prefix default for alt option is 'chr'\n", ChrDecodeShort,
           ChrDecodeLong);
    printf("  -%c\t: disassemble file (alt %s);"
           "\n\t  with verbose prints duplicate lines\n", DisassembleShort,
           DisassembleLong);
    printf("  -%c\t: print usage (alt %s)\n", HelpShort, HelpLong);
    printf("  -%c\t: print file cartridge info (alt %s);"
           "\n\t  with verbose prints more details\n", InfoShort, InfoLong);
    printf("  -%c\t: print version (alt %s)\n", VersionShort, VersionLong);
    puts("\narguments");
    puts("  file\t: input file containing cartridge or program contents");
    puts("\nhalt condition expressions");
    puts("  @XXXX\t: halt on instruction at address XXXX;"
         "\n\t  address is a 1- or 2-byte hexadecimal number");
    puts("  N.Ns\t: halt after running for N.N seconds;"
         "\n\t  non-resumable, useful in batch mode"
         " as a fail-safe halt condition");
    puts("  Nc\t: halt after executing N cycles");
    puts("  jam\t: halt when the CPU enters a jammed state");
}

void argparse_version(void)
{
    printf("Aldo %s", Version);
#ifdef __VERSION__
    fputs(" (" __VERSION__ ")", stdout);
#endif
    puts("");
}

void argparse_cleanup(struct control *appstate)
{
    for (struct haltarg *curr;
         appstate->haltlist;
         curr = appstate->haltlist,
            appstate->haltlist = appstate->haltlist->next,
            free(curr));
}

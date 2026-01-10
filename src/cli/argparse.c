//
//  argparse.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/23/21.
//

#include "argparse.h"

#include "bytes.h"
#include "cliargs.h"
#include "debug.h"
#include "dis.h"
#include "haltexpr.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char
    *const restrict BatchLong = "--batch",
    *const restrict BcdLong = "--bcd",
    *const restrict ChrDecodeLong = "--chr-decode",
    *const restrict ChrScaleLong = "--chr-scale",
    *const restrict DebugFileLong = "--dbg-file",
    *const restrict DisassembleLong = "--disassemble",
    *const restrict HaltLong = "--halt",
    *const restrict HelpLong = "--help",
    *const restrict InfoLong = "--info",
    *const restrict ResVectorLong = "--reset-vector",
    *const restrict TraceLong = "--trace",
    *const restrict VersionLong = "--version",
    *const restrict ZeroRamLong = "--zero-ram";

constexpr char BatchShort = 'b';
constexpr char BcdShort = 'D';
constexpr char ChrDecodeShort = 'c';
constexpr char ChrScaleShort = 's';
constexpr char DebugFileShort = 'g';
constexpr char DisassembleShort = 'd';
constexpr char HaltShort = 'H';
constexpr char HelpShort = 'h';
constexpr char InfoShort = 'i';
constexpr char ResVectorShort = 'r';
constexpr char TraceShort = 't';
constexpr char VerboseShort = 'v';
constexpr char VersionShort = 'V';
constexpr char ZeroRamShort = 'z';

constexpr int MinAddress = 0;
constexpr int MaxAddress = ALDO_ADDRMASK_64KB;

static void init_cliargs(struct cliargs *args)
{
    *args = (struct cliargs){
        .chrscale = Aldo_MinChrScale,
        .resetvector = Aldo_NoResetVector,
    };
}

static bool parse_flag(const char *arg, char shrt, bool exact, const char *lng)
{
    size_t lnglen = lng ? strlen(lng) : 0;
    return (strlen(arg) > 1
                && exact
                    ? arg[1] == shrt
                    : arg[1] != '-' && strchr(arg, shrt))
            || (lnglen > 0 && strncmp(arg, lng, lnglen) == 0
                && (arg[lnglen] == '\0' || arg[lnglen] == '='));
}

static bool convert_num(const char *restrict arg, int base, long *result)
{
    char *end;
    errno = 0;
    *result = strtol(arg, &end, base);
    if ((*result == LONG_MIN || *result == LONG_MAX) && errno == ERANGE) {
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
        const char *opt = strchr(arg, '=');
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
    bool result = parse_number(arg, argi, argc, argv, 16, &addr);
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
                       char *argv[argc+1], struct cliargs *restrict args)
{
    size_t optlen = strlen(HaltLong);
    const char *expr = nullptr;
    if (arg[1] == HaltShort && arg[2] != '\0') {
        expr = arg + 2;
    } else if (strncmp(arg, HaltLong, optlen) == 0) {
        const char *opt = strchr(arg, '=');
        if (opt && opt - arg == (ptrdiff_t)optlen) {
            expr = opt + 1;
        }
    }
    if (!expr && ++*argi < argc) {
        expr = argv[*argi];
    }
    if (expr) {
        struct haltarg **tail;
        for (tail = &args->haltlist; *tail; tail = &(*tail)->next);
        if ((*tail = malloc(sizeof **tail))) {
            **tail = (struct haltarg){.expr = expr};
            return true;
        }
        perror("Halt expression parse failed");
    }
    return false;
}

static bool parse_dbgfile(const char *arg, int *restrict argi, int argc,
                          char *argv[argc+1], struct cliargs *restrict args)
{
    size_t optlen = strlen(DebugFileLong);
    if (arg[1] == DebugFileShort && arg[2] != '\0') {
        args->dbgfilepath = arg + 2;
    } else if (strncmp(arg, DebugFileLong, optlen) == 0) {
        const char *opt = strchr(arg, '=');
        if (opt && opt - arg == (ptrdiff_t)optlen) {
            args->dbgfilepath = opt + 1;
        }
    }
    if (!args->dbgfilepath && ++*argi < argc) {
        args->dbgfilepath = argv[*argi];
    }
    return args->dbgfilepath;
}

static bool parse_arg(const char *arg, int *restrict argi, int argc,
                      char *argv[argc+1], struct cliargs *restrict args)
{
#define SETFLAG(f, a, s, l) ((f) = (f) || parse_flag(a, s, false, l))

    if (parse_flag(arg, ChrScaleShort, true, ChrScaleLong)) {
        long scale;
        bool result = parse_number(arg, argi, argc, argv, 10, &scale);
        if (result && Aldo_MinChrScale <= scale && scale <= Aldo_MaxChrScale) {
            args->chrscale = (int)scale;
            return true;
        }
        fprintf(stderr, "Invalid scale format: expected [%d, %d]\n",
                Aldo_MinChrScale, Aldo_MaxChrScale);
        return false;
    }

    if (parse_flag(arg, ResVectorShort, true, ResVectorLong)) {
        return parse_address(arg, argi, argc, argv, "vector",
                             &args->resetvector);
    }

    if (parse_flag(arg, HaltShort, true, HaltLong)) {
        return parse_halt(arg, argi, argc, argv, args);
    }

    if (parse_flag(arg, DebugFileShort, true, DebugFileLong)) {
        return parse_dbgfile(arg, argi, argc, argv, args);
    }

    SETFLAG(args->chrdecode, arg, ChrDecodeShort, ChrDecodeLong);
    size_t chroptlen = strlen(ChrDecodeLong);
    if (strncmp(arg, ChrDecodeLong, chroptlen) == 0) {
        const char *opt = strchr(arg, '=');
        if (opt && opt - arg == (ptrdiff_t)chroptlen) {
            args->chrdecode_prefix = opt + 1;
        }
    }

    SETFLAG(args->batch, arg, BatchShort, BatchLong);
    SETFLAG(args->bcdsupport, arg, BcdShort, BcdLong);
    SETFLAG(args->disassemble, arg, DisassembleShort, DisassembleLong);
    SETFLAG(args->help, arg, HelpShort, HelpLong);
    SETFLAG(args->info, arg, InfoShort, InfoLong);
    SETFLAG(args->tron, arg, TraceShort, TraceLong);
    SETFLAG(args->verbose, arg, VerboseShort, nullptr);
    SETFLAG(args->version, arg, VersionShort, VersionLong);
    SETFLAG(args->zeroram, arg, ZeroRamShort, ZeroRamLong);

    return true;

#undef SETFLAG
}

//
// MARK: - Public Interface
//

bool argparse_parse(struct cliargs *restrict args, int argc,
                    char *argv[argc+1])
{
    assert(args != nullptr);
    assert(argc > 0 ? argv != nullptr : true);

    init_cliargs(args);
    args->me = argc > 0 && strlen(argv[0]) > 0 ? argv[0] : "aldo";
    bool opt_parse = true;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const char *arg = argv[i];
            if (arg[0] == '-') {
                if (!opt_parse) {
                    continue;
                } else if (arg[1] == '-' && arg[2] == '\0') {
                    opt_parse = false;
                } else if (!parse_arg(arg, &i, argc, argv, args)) {
                    argparse_cleanup(args);
                    return false;
                }
            } else {
                args->filepath = arg;
            }
        }
    } else {
        args->help = true;
    }
    return true;
}

const char *argparse_filename(const char *filepath)
{
    if (!filepath) return nullptr;

    const char *last_slash = strrchr(filepath, '/');
    return last_slash ? last_slash + 1 : filepath;
}

void argparse_usage(const char *me)
{
    static const char
        *const restrict main_arg = "file",
        *const restrict program = "Aldo",
        *const restrict addr_desc = "address is a 1- or 2-byte hexadecimal number";

    char buf[5];
    int cpad = sizeof buf, spad = cpad + 1;

    printf("---=== %s Usage ===---\n", program);
    printf("%s [options...] [command] %s\n", me ? me : program, main_arg);

    puts("\noptions (--alt)");
    printf("  -%-*c: run program in batch mode (%s)\n", cpad, BatchShort,
           BatchLong);
    printf("  -%-*c: enable BCD (binary-coded decimal) support (%s)\n", cpad,
           BcdShort, BcdLong);
    sprintf(buf, "-%c f", DebugFileShort);
    printf("  %-*s: line-delimited debugger file containing halt conditions\n"
           "  %-*s  and/or RESET vector override (%s f)\n", spad, buf, spad,
           "", DebugFileLong);
    sprintf(buf, "-%c e", HaltShort);
    printf("  %-*s: halt condition expression (%s e);\n"
           "  %-*s  multiple -%c options can be specified,\n"
           "  %-*s  see below usage section for syntax\n", spad, buf,
           HaltLong, spad, "", HaltShort, spad, "");
    sprintf(buf, "-%c x", ResVectorShort);
    printf("  %-*s: override RESET vector [0x%X, 0x%X] (%s x)\n", spad, buf,
           MinAddress, MaxAddress, ResVectorLong);
    sprintf(buf, "-%c n", ChrScaleShort);
    printf("  %-*s: CHR ROM BMP scaling factor [%d, %d] (%s n)\n", spad, buf,
           Aldo_MinChrScale, Aldo_MaxChrScale, ChrScaleLong);
    printf("  -%-*c: turn on trace-logging and ram dumps (%s)\n", cpad,
           TraceShort, TraceLong);
    printf("  -%-*c: verbose output\n", cpad, VerboseShort);
    printf("  -%-*c: zero-out RAM on startup (%s)\n", cpad, ZeroRamShort,
           ZeroRamLong);

    puts("\ncommands (--alt)");
    printf("  -%-*c: decode CHR ROM into BMP files (%s[=prefix]);\n"
           "  %-*s  prefix default is 'chr'\n", cpad, ChrDecodeShort,
           ChrDecodeLong, spad, "");
    printf("  -%-*c: disassemble file (%s);\n"
           "  %-*s  with -%c for duplicate lines\n", cpad, DisassembleShort,
           DisassembleLong, spad, "", VerboseShort);
    printf("  -%-*c: print usage (%s)\n", cpad, HelpShort, HelpLong);
    printf("  -%-*c: print cartridge info (%s);\n"
           "  %-*s  with -%c for more detail\n", cpad, InfoShort, InfoLong,
           spad, "", VerboseShort);
    printf("  -%-*c: print version (%s)\n",  cpad, VersionShort, VersionLong);

    puts("\narguments");
    printf("  %-*s: input file containing cartridge or program contents\n",
           spad, main_arg);

    puts("\nhalt condition expressions");
    printf("  %-*s: halt on instruction at address XXXX;\n"
           "  %-*s  %s\n", spad, "@XXXX", spad, "", addr_desc);
    printf("  %-*s: halt after running for N.N seconds; non-resumable,\n"
           "  %-*s  useful in batch mode as a fail-safe halt condition\n",
           spad, "N.Ns", spad, "");
    printf("  %-*s: halt after executing N cycles\n", spad, "Nc");
    printf("  %-*s: halt after executing N frames\n", spad, "Nf");
    printf("  %-*s: halt when the CPU enters a jammed state\n", spad, "jam");

    puts("\nRESET vector override expression");
    printf("  %-*s: set RESET vector to address XXXX;\n"
           "  %-*s  %s\n", spad, ALDO_HEXPR_RST_IND "XXXX", spad, "",
           addr_desc);
}

void argparse_cleanup(struct cliargs *args)
{
    assert(args != nullptr);

    for (struct haltarg *curr;
         args->haltlist;
         curr = args->haltlist,
            args->haltlist = args->haltlist->next,
            free(curr));
}

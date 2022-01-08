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
    *const restrict Version = "0.3.0", // TODO: autogenerate this

    *const restrict BatchLong = "--batch",
    *const restrict ChrDecodeLong = "--chr-decode",
    *const restrict ChrScaleLong = "--chr-scale",
    *const restrict DisassembleLong = "--disassemble",
    *const restrict HaltLong = "--halt",
    *const restrict HelpLong = "--help",
    *const restrict InfoLong = "--info",
    *const restrict ResVectorLong = "--reset-vector",
    *const restrict TraceLong = "--trace",
    *const restrict VersionLong = "--version";

static const char
    BatchShort = 'b',
    ChrDecodeShort = 'c',
    ChrScaleShort = 's',
    DisassembleShort = 'd',
    HaltShort = 'H',
    HelpShort = 'h',
    InfoShort = 'i',
    ResVectorShort = 'r',
    TraceShort = 't',
    VerboseShort = 'v',
    VersionShort = 'V';

static const int
    MinScale = 1, MaxScale = 10, MinAddress = 0, MaxAddress = ADDRMASK_64KB;

static void init_control(struct control *appstate)
{
    *appstate = (struct control){
        .chrscale = MinScale,
        .clock = {.cycles_per_sec = 4},
        .resetvector = -1,
        .running = true,
    };
}

static bool parse_flag(const char *arg, char shrt, bool exact, const char *lng)
{
    return (strlen(arg) > 1
                && exact
                    ? arg[1] == shrt
                    : arg[1] != '-' && strchr(arg, shrt) != NULL)
            || (lng && strncmp(arg, lng, strlen(lng)) == 0);
}

#define SETFLAG(f, a, s, l) (f) = (f) || parse_flag(a, s, false, l)

static bool convert_num(const char *arg, int base, long *restrict result)
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

static bool parse_arg(struct control *restrict appstate, const char *arg,
                      int *restrict argi, int argc, char *argv[argc+1])
{
    if (parse_flag(arg, ChrScaleShort, true, ChrScaleLong)) {
        long scale;
        const bool result = parse_number(arg, argi, argc, argv, 10, &scale);
        if (result && MinScale <= scale && scale <= MaxScale) {
            appstate->chrscale = (int)scale;
        } else {
            fprintf(stderr, "Invalid scale format: expected [%d, %d]\n",
                    MinScale, MaxScale);
            return false;
        }
        return true;
    }

    if (parse_flag(arg, ResVectorShort, true, ResVectorLong)) {
        return parse_address(arg, argi, argc, argv, "vector",
                             &appstate->resetvector);
    }

    SETFLAG(appstate->batch, arg, BatchShort, BatchLong);
    SETFLAG(appstate->disassemble, arg, DisassembleShort, DisassembleLong);
    SETFLAG(appstate->help, arg, HelpShort, HelpLong);
    SETFLAG(appstate->info, arg, InfoShort, InfoLong);
    SETFLAG(appstate->tron, arg, TraceShort, TraceLong);
    SETFLAG(appstate->verbose, arg, VerboseShort, NULL);
    SETFLAG(appstate->version, arg, VersionShort, VersionLong);

    SETFLAG(appstate->chrdecode, arg, ChrDecodeShort, ChrDecodeLong);
    const size_t chroptlen = strlen(ChrDecodeLong);
    if (strncmp(arg, ChrDecodeLong, chroptlen) == 0) {
        const char *const opt = strchr(arg, '=');
        if (opt && opt - arg == chroptlen) {
            appstate->chrdecode_prefix = opt + 1;
        }
    }

    const size_t haltoptlen = strlen(HaltLong);
    if (parse_flag(arg, HaltShort, false, HaltLong)) {
        if (arg[1] == HaltShort && arg[2] != '\0') {
            appstate->haltexprs = arg + 2;
        } else if (strncmp(arg, HaltLong, haltoptlen) == 0) {
            const char *const opt = strchr(arg, '=');
            if (opt && opt - arg == haltoptlen) {
                appstate->haltexprs = opt + 1;
            }
        }
        if (!appstate->haltexprs && ++*argi < argc) {
            appstate->haltexprs = argv[*argi];
        }
        if (!appstate->haltexprs) return false;
    }
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
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const char *const arg = argv[i];
            if (arg[0] == '-') {
                if (!parse_arg(appstate, arg, &i, argc, argv)) return false;
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
    printf("  -%c\t: run program in batch mode (also %s)\n", BatchShort,
           BatchLong);
    printf("  -%c e\t: halt condition expressions;"
           " see below for syntax (also %s e)\n", HaltShort,
           HaltLong);
    printf("  -%c x\t: override RESET vector [0x%X, 0x%X]"
           " (also %s x)\n", ResVectorShort, MinAddress, MaxAddress,
           ResVectorLong);
    printf("  -%c n\t: CHR ROM BMP scaling factor [%d, %d]"
           " (also %s n)\n", ChrScaleShort, MinScale, MaxScale, ChrScaleLong);
    printf("  -%c\t: turn on trace-logging and ram dumps (also %s)\n",
           TraceShort, TraceLong);
    printf("  -%c\t: verbose output\n", VerboseShort);
    puts("\ncommands");
    printf("  -%c\t: decode CHR ROM into BMP files (also %s[=prefix];"
           " prefix default is 'bank')\n", ChrDecodeShort, ChrDecodeLong);
    printf("  -%c\t: disassemble file (also %s);"
           " verbose prints duplicate lines\n", DisassembleShort,
           DisassembleLong);
    printf("  -%c\t: print usage (also %s)\n", HelpShort, HelpLong);
    printf("  -%c\t: print file cartridge info (also %s);"
           " verbose prints more details\n", InfoShort, InfoLong);
    printf("  -%c\t: print version (also %s)\n", VersionShort, VersionLong);
    puts("\narguments");
    puts("  file\t: input file containing cartridge"
         " or program contents");
}

void argparse_version(void)
{
    printf("Aldo %s", Version);
#ifdef __VERSION__
    fputs(" (" __VERSION__ ")", stdout);
#endif
    puts("");
}

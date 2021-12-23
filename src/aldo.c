//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "bytes.h"
#include "cart.h"
#include "control.h"
#include "dis.h"
#include "nes.h"
#include "ui.h"
#include "snapshot.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char
    *restrict const Version = "0.2.1", // TODO: autogenerate this

    *restrict const ChrDecodeLong = "--chr-decode",
    *restrict const ChrScaleLong = "--chr-scale",
    *restrict const DisassembleLong = "--disassemble",
    *restrict const HelpLong = "--help",
    *restrict const InfoLong = "--info",
    *restrict const ResVectorLong = "--reset-vector",
    *restrict const VersionLong = "--version";

static const char
    ChrDecodeShort = 'c',
    ChrScaleShort = 's',
    DisassembleShort = 'd',
    HelpShort = 'h',
    InfoShort = 'i',
    ResVectorShort = 'r',
    VerboseShort = 'v',
    VersionShort = 'V';

static const int
    ArgParseFailure = -1, MinScale = 1, MaxScale = 10, MinVector = 0,
    MaxVector = ADDRMASK_64KB;

static bool parse_flag(const char *arg, char shrt, bool exact,
                       const char *restrict lng)
{
    return (strlen(arg) > 1
                && exact
                    ? arg[1] == shrt
                    : arg[1] != '-' && strchr(arg, shrt) != NULL)
            || (lng && strcmp(arg, lng) == 0);
}

#define setflag(f, a, s, l) (f) = (f) || parse_flag(a, s, false, l)

static bool convert_num(const char *arg, int base, long *result)
{
    char *end;
    errno = 0;
    *result = strtol(arg, &end, base);
    if (errno == ERANGE) {
        perror("Number parse failed");
        return false;
    }
    if (*result == 0 && arg == end) {
        return false;
    }
    return true;
}

// NOTE: value of number is undefined if return value is false
static bool parse_number(const char *arg, int *argi, int argc,
                         char *argv[argc+1], int base, long *number)
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

static int parse_args(struct control *appstate, int argc, char *argv[argc+1])
{
    appstate->me = argc > 0 && strlen(argv[0]) > 0 ? argv[0] : "aldo";
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const char *const arg = argv[i];
            if (arg[0] == '-') {
                if (parse_flag(arg, ChrScaleShort, true, ChrScaleLong)) {
                    long scale;
                    const bool result = parse_number(arg, &i, argc, argv, 10,
                                                     &scale);
                    if (result && MinScale <= scale && scale <= MaxScale) {
                        appstate->chrscale = (int)scale;
                    } else {
                        fprintf(stderr,
                                "Invalid scale format: expected [%d, %d]\n",
                                MinScale, MaxScale);
                        return ArgParseFailure;
                    }
                    continue;
                }
                if (parse_flag(arg, ResVectorShort, true, ResVectorLong)) {
                    long vector;
                    const bool result = parse_number(arg, &i, argc, argv, 16,
                                                     &vector);
                    if (result && MinVector <= vector && vector <= MaxVector) {
                        appstate->resetvector = (int)vector;
                    } else {
                        fprintf(stderr,
                                "Invalid vector format: "
                                "expected [0x%X, 0x%X]\n",
                                MinVector, MaxVector);
                        return ArgParseFailure;
                    }
                    continue;
                }

                setflag(appstate->disassemble, arg, DisassembleShort,
                        DisassembleLong);
                setflag(appstate->help, arg, HelpShort, HelpLong);
                setflag(appstate->info, arg, InfoShort, InfoLong);
                setflag(appstate->verbose, arg, VerboseShort, NULL);
                setflag(appstate->version, arg, VersionShort, VersionLong);

                setflag(appstate->chrdecode, arg, ChrDecodeShort,
                        ChrDecodeLong);
                const size_t chroptlen = strlen(ChrDecodeLong);
                if (strncmp(arg, ChrDecodeLong, chroptlen) == 0) {
                    const char *const opt = strchr(arg, '=');
                    if (opt && opt - arg == chroptlen) {
                        appstate->chrdecode_prefix = opt + 1;
                    }
                }
            } else {
                appstate->cartfile = arg;
            }
        }
    } else {
        appstate->help = true;
    }

    return 0;
}

static void print_usage(const struct control *appstate)
{
    puts("---=== Aldo Usage ===---");
    printf("%s [options...] [command] file\n", appstate->me);
    puts("\noptions");
    printf("  -%c x\t: override RESET vector [0x%X, 0x%X]"
           " (also %s x)\n", ResVectorShort, MinVector, MaxVector,
           ResVectorLong);
    printf("  -%c n\t: CHR ROM BMP scaling factor [%d, %d]"
           " (also %s n)\n", ChrScaleShort, MinScale, MaxScale, ChrScaleLong);
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

static void print_version(void)
{
    printf("Aldo %s", Version);
#ifdef __VERSION__
    fputs(" (" __VERSION__ ")", stdout);
#endif
    puts("");
}

static cart *load_cart(const char *filename)
{
    cart *c = NULL;
    errno = 0;
    FILE *const cartfile = fopen(filename, "rb");
    if (!cartfile) {
        perror("Cannot open cart file");
        return c;
    }

    const int err = cart_create(&c, cartfile);
    if (err < 0) {
        fprintf(stderr, "Cart load failure (%d): %s\n", err, cart_errstr(err));
        if (err == CART_ERR_IO) {
            perror("Cart IO error");
        }
    }
    fclose(cartfile);

    return c;
}

static int print_cart_info(const struct control *appstate, cart *c)
{
    if (appstate->verbose) {
        puts("---=== Cart Info ===---");
    }
    printf("File\t\t: %s\n", ctrl_cartfilename(appstate->cartfile));
    cart_write_info(c, stdout, appstate->verbose);
    return EXIT_SUCCESS;
}

static int disassemble_cart_prg(const struct control *appstate, cart *c)
{
    return dis_cart_prg(c, appstate, stdout) == 0
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
}

static int decode_cart_chr(const struct control *appstate, cart *c)
{
    const int err = dis_cart_chr(c, appstate, stdout);
    if (err < 0) {
        fprintf(stderr, "CHR decode error (%d): %s\n", err,
                dis_errstr(err));
        if (err == DIS_ERR_IO) {
            perror("CHR decode file error");
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static void handle_input(struct control *appstate,
                         const struct console_state *snapshot, nes *console)
{
    const int c = ui_pollinput();
    switch (c) {
    case ' ':
        if (snapshot->lines.ready) {
            nes_halt(console);
        } else {
            nes_ready(console);
        }
        break;
    case '=':   // "Lowercase" +
        ++appstate->clock.cycles_per_sec;
        goto pclamp_cps;
    case '+':
        appstate->clock.cycles_per_sec += 10;
    pclamp_cps:
        if (appstate->clock.cycles_per_sec > MaxCps) {
            appstate->clock.cycles_per_sec = MaxCps;
        }
        break;
    case '-':
        --appstate->clock.cycles_per_sec;
        goto nclamp_cps;
    case '_':   // "Uppercase" -
        appstate->clock.cycles_per_sec -= 10;
    nclamp_cps:
        if (appstate->clock.cycles_per_sec < MinCps) {
            appstate->clock.cycles_per_sec = MinCps;
        }
        break;
    case 'i':
        if (snapshot->lines.irq) {
            nes_interrupt(console, NESI_IRQ);
        } else {
            nes_clear(console, NESI_IRQ);
        }
        break;
    case 'm':
        nes_mode(console, snapshot->mode + 1);
        break;
    case 'M':
        nes_mode(console, snapshot->mode - 1);
        break;
    case 'n':
        if (snapshot->lines.nmi) {
            nes_interrupt(console, NESI_NMI);
        } else {
            nes_clear(console, NESI_NMI);
        }
        break;
    case 'q':
        appstate->running = false;
        break;
    case 'r':
        appstate->ramsheet = (appstate->ramsheet + 1) % RamSheets;
        break;
    case 'R':
        --appstate->ramsheet;
        if (appstate->ramsheet < 0) {
            appstate->ramsheet = RamSheets - 1;
        }
        break;
    case 's':
        if (snapshot->lines.reset) {
            nes_interrupt(console, NESI_RES);
        } else {
            nes_clear(console, NESI_RES);
        }
        break;
    }
}

static void update(struct control *appstate, struct console_state *snapshot,
                   nes *console)
{
    nes_cycle(console, &appstate->clock);
    nes_snapshot(console, snapshot);
    ui_refresh(appstate, snapshot);
}

static int emu_loop(struct control *appstate, cart *c)
{
    FILE *tracelog = NULL;
    if (appstate->tron) {
        if (!(tracelog = fopen(appstate->tracefile, "w"))) {
            fprintf(stderr, "%s: ", appstate->tracefile);
            perror("Cannot open trace file");
            return EXIT_FAILURE;
        }
    }

    nes *console = nes_new(c, tracelog, appstate->resetvector);
    nes_powerup(console);
    // NOTE: initialize snapshot from console
    struct console_state snapshot;
    nes_snapshot(console, &snapshot);
    ui_init();

    do {
        ui_tick_start(appstate, &snapshot);
        handle_input(appstate, &snapshot, console);
        if (appstate->running) {
            update(appstate, &snapshot, console);
        }
        ui_tick_end();
    } while (appstate->running);

    ui_cleanup();
    snapshot.mem.prglength = 0;
    snapshot.mem.ram = NULL;
    nes_free(console);
    console = NULL;
    if (tracelog) {
        fclose(tracelog);
        tracelog = NULL;
    }
    return EXIT_SUCCESS;
}

//
// Public Interface
//

int aldo_run(int argc, char *argv[argc+1])
{
    struct control appstate = {
        .chrscale = MinScale,
        .clock = {.cycles_per_sec = 4},
        .resetvector = -1,
        .running = true,
        .tracefile = "trace.log",
        .tron = true,
    };
    if (parse_args(&appstate, argc, argv)
        == ArgParseFailure) return EXIT_FAILURE;

    if (appstate.help) {
        print_usage(&appstate);
        return EXIT_SUCCESS;
    }

    if (appstate.version) {
        print_version();
        return EXIT_SUCCESS;
    }

    if (!appstate.cartfile) {
        fputs("Error: no input file specified\n", stderr);
        print_usage(&appstate);
        return EXIT_FAILURE;
    }

    cart *cart = load_cart(appstate.cartfile);
    if (!cart) {
        return EXIT_FAILURE;
    }

    int result;

    if (appstate.info) {
        result = print_cart_info(&appstate, cart);
        goto cart_cleanup;
    }

    if (appstate.disassemble) {
        result = disassemble_cart_prg(&appstate, cart);
        goto cart_cleanup;
    }

    if (appstate.chrdecode) {
        result = decode_cart_chr(&appstate, cart);
        goto cart_cleanup;
    }

    result = emu_loop(&appstate, cart);

cart_cleanup:
    cart_free(cart);
    cart = NULL;

    return result;
}

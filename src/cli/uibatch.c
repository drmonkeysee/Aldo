//
//  uibatch.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "argparse.h"
#include "cycleclock.h"
#include "debug.h"
#include "emu.h"
#include "haltexpr.h"
#include "nes.h"
#include "tsutil.h"
#include "ui.h"

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *const restrict DistractorFormat = "%c Running\u2026";

struct runclock {
    struct cycleclock cyclock;
    double avgframetime_ms;
};

static void clearline(void)
{
    const size_t linelength = strlen(DistractorFormat) * 2;
    for (size_t i = 0; i < linelength; ++i) {
        fputc('\b', stderr);
    }
}

static volatile sig_atomic_t QuitSignal;

static void handle_sigint(int sig, siginfo_t *info, void *uap)
{
    (void)sig, (void)info, (void)uap;
    QuitSignal = 1;
}

//
// UI Loop Implementation
//

static int init_ui(void)
{
    struct sigaction act = {
        .sa_sigaction = handle_sigint,
        .sa_flags = SA_SIGINFO,
    };
    return sigaction(SIGINT, &act, NULL) == 0 ? 0 : UI_ERR_ERNO;
}

static void tick_start(const struct emulator *emu, struct runclock *c)
{
    cycleclock_tickstart(&c->cyclock, true);

    // NOTE: cumulative moving average:
    // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
    const double ticks = (double)c->cyclock.frames;
    c->avgframetime_ms = (c->cyclock.frametime_ms
                          + (ticks * c->avgframetime_ms)) / (ticks + 1);

    // NOTE: arbitrary per-tick budget, 6502s often ran at 1 MHz so a million
    // cycles per tick seems as good a number as any.
    c->cyclock.budget = 1e6;
    // NOTE: app runtime and emulator time are equivalent in batch mode
    c->cyclock.emutime = c->cyclock.runtime;

    // NOTE: exit batch mode if cpu is not running
    if (!emu->snapshot.lines.ready) {
        QuitSignal = 1;
    }
}

static void emu_update(struct emulator *emu, struct runclock *c)
{
    nes_cycle(emu->console, &c->cyclock);
    nes_snapshot(emu->console, &emu->snapshot);
}

static void update_progress(const struct runclock *c)
{
    static const char distractor[] = {'|', '/', '-', '\\'};
    static const double display_wait = 2000,
                        refresh_interval_ms = display_wait + 100;
    static double refreshdt;
    static size_t distractor_frame;

    if ((refreshdt += c->cyclock.frametime_ms) >= refresh_interval_ms) {
        refreshdt = display_wait;
        clearline();
        fprintf(stderr, DistractorFormat, distractor[distractor_frame++]);
        distractor_frame %= sizeof distractor;
    }
}

static void tick_end(struct runclock *c)
{
    cycleclock_tickend(&c->cyclock);
}

static void write_summary(const struct emulator *emu, const struct runclock *c)
{
    clearline();
    if (!emu->args->verbose) return;

    const bool scale_ms = c->cyclock.runtime < 1.0;
    printf("---=== %s ===---\n", argparse_filename(emu->args->filepath));
    printf("Runtime (%ssec): %.3f\n", scale_ms ? "m" : "",
           scale_ms
            ? c->cyclock.runtime * TSU_MS_PER_S
            : c->cyclock.runtime);
    printf("Avg Frame Time (msec): %.3f\n", c->avgframetime_ms);
    printf("Total Cycles: %" PRIu64 "\n", c->cyclock.total_cycles);
    printf("Avg Cycles/sec: %.2f\n",
           (double)c->cyclock.total_cycles / c->cyclock.runtime);
    if (emu->snapshot.debugger.halted != NoBreakpoint) {
        const struct breakpoint *const bp =
            debug_bp_at(emu->dbg, emu->snapshot.debugger.halted);
        assert(bp != NULL);
        char break_desc[HEXPR_FMT_SIZE];
        const int err = haltexpr_desc(&bp->expr, break_desc);
        printf("Break: %s\n", err < 0 ? haltexpr_errstr(err) : break_desc);
    }
}

//
// Public Interface
//

int ui_batch_loop(struct emulator *emu)
{
    assert(emu != NULL);

    const int err = init_ui();
    if (err < 0) return err;

    struct runclock clock = {0};
    cycleclock_start(&clock.cyclock);
    do {
        tick_start(emu, &clock);
        emu_update(emu, &clock);
        update_progress(&clock);
        tick_end(&clock);
    } while (QuitSignal == 0);
    write_summary(emu, &clock);

    return 0;
}

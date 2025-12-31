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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *const restrict DistractorFormat = "%c Running\u2026";

struct runclock {
    struct aldo_clock clock;
    double avg_ticktime_ms;
};

static void clearline()
{
    size_t linelength = strlen(DistractorFormat) * 2;
    for (size_t i = 0; i < linelength; ++i) {
        fputc('\b', stderr);
    }
}

static volatile sig_atomic_t QuitSignal;

static void handle_sigint(int, siginfo_t *, void *)
{
    QuitSignal = 1;
}

//
// MARK: - UI Loop Implementation
//

static int init_ui()
{
    struct sigaction act = {
        .sa_sigaction = handle_sigint,
        .sa_flags = SA_SIGINFO,
    };
    return sigaction(SIGINT, &act, nullptr) == 0 ? 0 : ALDO_UI_ERR_ERNO;
}

static void tick_start(struct runclock *c, const struct aldo_snapshot *snp)
{
    aldo_clock_tickstart(&c->clock, true);

    // NOTE: cumulative moving average:
    // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
    double ticks = (double)c->clock.ticks;
    c->avg_ticktime_ms = (c->clock.ticktime_ms + (ticks * c->avg_ticktime_ms))
                            / (ticks + 1);

    // NOTE: arbitrary per-tick budget, 6502s often ran at 1 MHz so a million
    // cycles per tick seems as good a number as any.
    c->clock.budget = 1e6;
    // NOTE: app runtime and emulator time are equivalent in batch mode
    c->clock.emutime = c->clock.runtime;

    // NOTE: exit batch mode if cpu is not running
    if (!snp->cpu.lines.ready) {
        QuitSignal = 1;
    }
}

static void update_progress(const struct runclock *c)
{
    static constexpr char distractor[] = {'|', '/', '-', '\\'};
    static constexpr double display_wait = 2000,
                            refresh_interval_ms = display_wait + 100;
    static double refreshdt;
    static size_t distractor_frame;

    if ((refreshdt += c->clock.ticktime_ms) >= refresh_interval_ms) {
        refreshdt = display_wait;
        clearline();
        fprintf(stderr, DistractorFormat, distractor[distractor_frame++]);
        distractor_frame %= sizeof distractor;
    }
}

static void tick_end(struct runclock *c)
{
    aldo_clock_tickend(&c->clock);
}

static void write_summary(const struct emulator *emu, const struct runclock *c)
{
    clearline();
    if (!emu->args->verbose) return;

    bool scale_ms = c->clock.runtime < 1;
    printf("---=== %s ===---\n", argparse_filename(emu->args->filepath));
    printf("Runtime (%ssec): %.3f\n", scale_ms ? "m" : "",
           scale_ms ? c->clock.runtime * ALDO_MS_PER_S : c->clock.runtime);
    printf("Avg Tick Time (msec): %.3f\n", c->avg_ticktime_ms);
    printf("Total Cycles: %" PRIu64 "\n", c->clock.cycles);
    printf("Avg Cycles/sec: %.2f\n",
           (double)c->clock.cycles / c->clock.runtime);
    auto bp = aldo_debug_halted(emu->debugger);
    if (bp) {
        char break_desc[ALDO_HEXPR_FMT_SIZE];
        int err = aldo_haltexpr_desc(&bp->expr, break_desc);
        printf("Break: %s\n",
               err < 0 ? aldo_haltexpr_errstr(err) : break_desc);
    }
}

//
// MARK: - Public Interface
//

int ui_batch_loop(struct emulator *emu)
{
    assert(emu != nullptr);

    int err = init_ui();
    if (err < 0) return err;

    struct runclock clock = {};
    aldo_clock_start(&clock.clock);
    do {
        tick_start(&clock, &emu->snapshot);
        aldo_nes_clock(emu->console, &clock.clock);
        update_progress(&clock);
        tick_end(&clock);
    } while (QuitSignal == 0);
    write_summary(emu, &clock);

    return 0;
}

//
//  uibatch.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "cart.h"
#include "cliargs.h"
#include "cycleclock.h"
#include "haltexpr.h"
#include "nes.h"
#include "snapshot.h"
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
#include <time.h>

static const char *const restrict DistractorFormat = "%c Running\u2026";

struct runclock {
    struct timespec current, previous, start;
    struct cycleclock cyclock;
    double avgframetime_ms, frametime_ms;
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

static int init_ui(struct runclock *clock)
{
    clock_gettime(CLOCK_MONOTONIC, &clock->start);
    clock->previous = clock->start;
    struct sigaction act = {
        .sa_sigaction = handle_sigint,
        .sa_flags = SA_SIGINFO,
    };
    return sigaction(SIGINT, &act, NULL) == 0 ? 0 : UI_ERR_ERNO;
}

static void tick_start(const struct console_state *snapshot,
                       struct runclock *clock)
{
    clock_gettime(CLOCK_MONOTONIC, &clock->current);
    const double currentms = timespec_to_ms(&clock->current);
    clock->frametime_ms = currentms - timespec_to_ms(&clock->previous);
    clock->cyclock.runtime = (currentms - timespec_to_ms(&clock->start))
                                / TSU_MS_PER_S;

    // NOTE: cumulative moving average:
    // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
    const double ticks = (double)clock->cyclock.frames;
    clock->avgframetime_ms = (clock->frametime_ms
                                    + (ticks * clock->avgframetime_ms))
                                / (ticks + 1);

    // NOTE: arbitrary per-tick budget, 6502s often ran at 1 MHz so a million
    // cycles per tick seems as good a number as any.
    clock->cyclock.budget = 1e6;

    // NOTE: exit batch mode if cpu is not running
    if (!snapshot->lines.ready) {
        QuitSignal = 1;
    }
}

static void tick_end(struct runclock *clock)
{
    clock->previous = clock->current;
    ++clock->cyclock.frames;
}

static void update_progress(const struct runclock *clock)
{
    static const char distractor[] = {'|', '/', '-', '\\'};
    static const double display_wait = 2000,
                        refresh_interval_ms = display_wait + 100;
    static double refreshdt;
    static size_t distractor_frame;

    if ((refreshdt += clock->frametime_ms) >= refresh_interval_ms) {
        refreshdt = display_wait;
        clearline();
        fprintf(stderr, DistractorFormat, distractor[distractor_frame++]);
        distractor_frame %= sizeof distractor;
    }
}

static void write_summary(const struct cliargs *args,
                          const struct console_state *snapshot,
                          const struct runclock *clock)
{
    clearline();
    if (!args->verbose) return;

    const bool scale_ms = clock->cyclock.runtime < 1.0;
    printf("---=== %s ===---\n", cart_filename(snapshot->cart.info));
    printf("Runtime (%ssec): %.3f\n", scale_ms ? "m" : "",
           scale_ms
            ? clock->cyclock.runtime * TSU_MS_PER_S
            : clock->cyclock.runtime);
    printf("Avg Frame Time (msec): %.3f\n", clock->avgframetime_ms);
    printf("Total Cycles: %" PRIu64 "\n", clock->cyclock.total_cycles);
    printf("Avg Cycles/sec: %.2f\n",
           (double)clock->cyclock.total_cycles / clock->cyclock.runtime);
    if (snapshot->debugger.break_condition.cond != HLT_NONE) {
        char break_desc[HEXPR_FMT_SIZE];
        const int err = haltexpr_fmt(&snapshot->debugger.break_condition,
                                     break_desc);
        printf("Break: %s\n", err < 0 ? haltexpr_errstr(err) : break_desc);
    }
}

//
// Public Interface
//

int ui_batch_loop(const struct cliargs *args, nes *console,
                  struct console_state *snapshot)
{
    assert(args != NULL);
    assert(console != NULL);
    assert(snapshot != NULL);

    struct runclock clock;
    const int err = init_ui(&clock);
    if (err < 0) return err;

    do {
        tick_start(snapshot, &clock);
        nes_cycle(console, &clock.cyclock);
        nes_snapshot(console, snapshot);
        update_progress(&clock);
        tick_end(&clock);
    } while (QuitSignal == 0);
    write_summary(args, snapshot, &clock);

    return 0;
}

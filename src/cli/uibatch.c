//
//  uibatch.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "ui.h"

#include "cart.h"
#include "cliargs.h"
#include "cycleclock.h"
#include "haltexpr.h"
#include "tsutil.h"

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

static struct {
    struct timespec current, previous, start;
    struct cycleclock cyclock;
    double avgframetime_ms, frametime_ms;
} Clock;
static bool Verbose;

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

static int init_ui(const struct cliargs *args)
{
    Verbose = args->verbose;
    clock_gettime(CLOCK_MONOTONIC, &Clock.start);
    Clock.previous = Clock.start;
    struct sigaction act = {
        .sa_sigaction = handle_sigint,
        .sa_flags = SA_SIGINFO,
    };
    return sigaction(SIGINT, &act, NULL) == 0 ? 0 : UI_ERR_ERNO;
}

static void tick_start(const struct console_state *snapshot)
{
    clock_gettime(CLOCK_MONOTONIC, &Clock.current);
    const double currentms = timespec_to_ms(&Clock.current);
    Clock.frametime_ms = currentms - timespec_to_ms(&Clock.previous);
    Clock.cyclock.runtime = (currentms - timespec_to_ms(&Clock.start))
                                / TSU_MS_PER_S;

    // NOTE: cumulative moving average:
    // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
    const double ticks = (double)Clock.cyclock.frames;
    Clock.avgframetime_ms = (Clock.frametime_ms
                             + (ticks * Clock.avgframetime_ms)) / (ticks + 1);

    // NOTE: arbitrary per-tick budget, 6502s often ran at 1 MHz so a million
    // cycles per tick seems as good a number as any.
    Clock.cyclock.budget = 1e6;

    // NOTE: exit batch mode if cpu is not running
    if (!snapshot->lines.ready) {
        QuitSignal = 1;
    }
}

static void tick_end(void)
{
    Clock.previous = Clock.current;
    ++Clock.cyclock.frames;
}

static void update_progress(void)
{
    static const char distractor[] = {'|', '/', '-', '\\'};
    static const double display_wait = 2000,
                        refresh_interval_ms = display_wait + 100;
    static double refreshdt;
    static size_t distractor_frame;

    if ((refreshdt += Clock.frametime_ms) >= refresh_interval_ms) {
        refreshdt = display_wait;
        clearline();
        fprintf(stderr, DistractorFormat, distractor[distractor_frame++]);
        distractor_frame %= sizeof distractor;
    }
}

static void write_summary(const struct console_state *snapshot)
{
    if (!Verbose) return;

    const bool scale_ms = Clock.cyclock.runtime < 1.0;

    clearline();
    printf("---=== %s ===---\n", cart_filename(snapshot->cart.info));
    printf("Runtime (%ssec): %.3f\n", scale_ms ? "m" : "",
           scale_ms
            ? Clock.cyclock.runtime * TSU_MS_PER_S
            : Clock.cyclock.runtime);
    printf("Avg Frame Time (msec): %.3f\n", Clock.avgframetime_ms);
    printf("Total Cycles: %" PRIu64 "\n", Clock.cyclock.total_cycles);
    printf("Avg Cycles/sec: %.2f\n",
           (double)Clock.cyclock.total_cycles / Clock.cyclock.runtime);
    if (snapshot->debugger.break_condition.cond != HLT_NONE) {
        char break_desc[HEXPR_FMT_SIZE];
        const int err = haltexpr_fmt(&snapshot->debugger.break_condition,
                                     break_desc);
        printf("Break: %s\n", err < 0 ? haltexpr_errstr(err) : break_desc);
    }
}

static void batch_loop(nes *console, struct console_state *snapshot)
{
    assert(console != NULL);
    assert(snapshot != NULL);

    do {
        tick_start(snapshot);
        nes_cycle(console, &Clock.cyclock);
        nes_snapshot(console, snapshot);
        update_progress();
        tick_end();
    } while (QuitSignal == 0);
    write_summary(snapshot);
}

//
// Public Interface
//

int ui_batch_init(const struct cliargs *args, ui_loop **loop)
{
    assert(args != NULL);
    assert(loop != NULL);

    const int err = init_ui(args);
    *loop = err == 0 ? batch_loop : NULL;
    return err;
}

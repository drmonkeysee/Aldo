//
//  uibatch.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "control.h"
#include "haltexpr.h"
#include "snapshot.h"
#include "tsutil.h"
#include "ui.h"

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *const restrict DistractorFormat = "%c Running\u2026";

static struct timespec Current, Previous, Start;
static double AvgFrameTimeMs, FrameTimeMs;
static uint64_t Ticks;

static void clearline(void)
{
    for (size_t i = 0; i < strlen(DistractorFormat) * 2; ++i) {
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
// UI Interface Implementation
//

static int batch_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &Start);
    Previous = Start;
    struct sigaction act = {
        .sa_sigaction = handle_sigint,
        .sa_flags = SA_SIGINFO,
    };
    return sigaction(SIGINT, &act, NULL) == 0 ? 0 : UI_ERR_ERNO;
}

static void batch_tick_start(struct control *appstate,
                             const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    clock_gettime(CLOCK_MONOTONIC, &Current);
    const double currentms = timespec_to_ms(&Current);
    FrameTimeMs = currentms - timespec_to_ms(&Previous);
    appstate->clock.runtime = (currentms - timespec_to_ms(&Start))
                                / TSU_MS_PER_S;

    // NOTE: cumulative moving average:
    // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
    AvgFrameTimeMs = (FrameTimeMs + (Ticks * AvgFrameTimeMs)) / (Ticks + 1);

    // NOTE: arbitrary per-tick budget, 6502s often ran at 1 MHz so a million
    // cycles per tick seems as good a number as any.
    appstate->clock.budget = 1e6;

    // NOTE: if cpu halts or jams, exit batch mode
    if (!snapshot->lines.ready || snapshot->datapath.jammed) {
        appstate->running = false;
    }
}

static void batch_tick_end(void)
{
    Previous = Current;
    ++Ticks;
}

static int batch_pollinput(void)
{
    return QuitSignal == 0 ? 0 : 'q';
}

static void batch_refresh(const struct control *appstate,
                          const struct console_state *snapshot)
{
    static const char distractor[] = {'|', '/', '-', '\\'};
    static const double display_wait = 2000,
                        refresh_interval_ms = display_wait + 100;
    static double refreshdt;
    static size_t distractor_frame;

    (void)appstate, (void)snapshot;
    if ((refreshdt += FrameTimeMs) >= refresh_interval_ms) {
        refreshdt = display_wait;
        clearline();
        fprintf(stderr, DistractorFormat, distractor[distractor_frame++]);
        distractor_frame %= sizeof distractor;
    }
}

static void batch_cleanup(const struct control *appstate,
                          const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    if (!appstate->verbose) return;

    const bool scale_ms = appstate->clock.runtime < 1.0;

    clearline();
    printf("---=== %s ===---\n", ctrl_cartfilename(appstate->cartfile));
    printf("Runtime (%ssec): %.3f\n", scale_ms ? "m" : "",
           scale_ms
            ? appstate->clock.runtime * TSU_MS_PER_S
            : appstate->clock.runtime);
    printf("Avg Frame Time (msec): %.3f\n", AvgFrameTimeMs);
    printf("Total Cycles: %" PRIu64 "\n", appstate->clock.total_cycles);
    printf("Avg Cycles/sec: %.2f\n",
           appstate->clock.total_cycles / appstate->clock.runtime);
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

int ui_batch_init(struct ui_interface *ui)
{
    assert(ui != NULL);

    const int result = batch_init();
    if (result == 0) {
        *ui = (struct ui_interface){
            batch_tick_start,
            batch_tick_end,
            batch_pollinput,
            batch_refresh,
            batch_cleanup,
        };
    }
    return result;
}

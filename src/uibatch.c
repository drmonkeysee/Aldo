//
//  uibatch.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "control.h"
#include "snapshot.h"
#include "tsutil.h"
#include "ui.h"

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *const restrict DistractorFormat = "%c Running\u2026";

static struct timespec Current, Previous, Start;
static double FrameTimeMs;

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
    FrameTimeMs = timespec_to_ms(&Current) - timespec_to_ms(&Previous);

    // NOTE: 1 cycle per tick approximates running emu at native cpu speed
    appstate->clock.budget = 1;

    // NOTE: if cpu halts or jams, exit batch mode
    if (!snapshot->lines.ready || snapshot->datapath.jammed) {
        appstate->running = false;
    }
}

static void batch_tick_end(void)
{
    Previous = Current;
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

    const struct timespec elapsed = timespec_elapsed(&Start);
    const double runtime = timespec_to_ms(&elapsed);
    const bool scale_ms = runtime < TSU_MS_PER_S;

    clearline();
    printf("---=== %s ===---\n", ctrl_cartfilename(appstate->cartfile));
    printf("Run Time (%ssec): %.3f\n", scale_ms ? "m" : "",
           scale_ms ? runtime : runtime / TSU_MS_PER_S);
    printf("Total Cycles: %" PRIu64 "\n", appstate->clock.total_cycles);
    printf("Avg Cycles/sec: %.2f\n",
           appstate->clock.total_cycles * (TSU_MS_PER_S / runtime));
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

//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

#include "asm.h"
#include "emu/bytes.h"
#include "emu/traits.h"

#include <ncurses.h>
#include <panel.h>

#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

struct view {
    WINDOW *restrict win, *restrict content;
    PANEL *restrict outer, *restrict inner;
};

//
// Run Loop Clock
//

static const double MillisecondsPerSecond = 1000,
                    NanosecondsPerMillisecond = 1e6;
static const long NanosecondsPerSecond = MillisecondsPerSecond
                                         * NanosecondsPerMillisecond;
// Approximate a 60hz run loop
static const int Fps = 60;
static const struct timespec VSync = {.tv_nsec = NanosecondsPerSecond / Fps};

static struct timespec Current, Previous;
static double CycleBudgetMs, FrameTimeMs, FrameLeftMs;

static double to_ms(const struct timespec *ts)
{
    return (ts->tv_sec * MillisecondsPerSecond)
            + (ts->tv_nsec / NanosecondsPerMillisecond);
}

static void initclock(void)
{
    clock_gettime(CLOCK_MONOTONIC, &Previous);
}

static void tick_elapsed(struct timespec *ts)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    *ts = (struct timespec){.tv_sec = now.tv_sec - Current.tv_sec};

    if (Current.tv_nsec > now.tv_nsec) {
        // NOTE: subtract with borrow
        --ts->tv_sec;
        ts->tv_nsec = NanosecondsPerSecond - (Current.tv_nsec - now.tv_nsec);
    } else {
        ts->tv_nsec = now.tv_nsec - Current.tv_nsec;
    }
}

static void sleeploop(void)
{
    struct timespec elapsed;
    tick_elapsed(&elapsed);

    // NOTE: if elapsed nanoseconds is greater than vsync we're over
    // our time budget; if elapsed seconds is greater than vsync
    // we've BLOWN AWAY our time budget; either way don't sleep.
    if (elapsed.tv_nsec > VSync.tv_nsec ||
        elapsed.tv_sec > VSync.tv_sec) return;

    const struct timespec tick_left = {
        .tv_nsec = VSync.tv_nsec - elapsed.tv_nsec
    };
    FrameLeftMs = to_ms(&tick_left);
    // TODO: use clock_nanosleep for Linux?
    // TODO: handle EINTR
    nanosleep(&tick_left, NULL);
}

//
// UI Widgets
//

static struct view HwView;
static struct view ControlsView;
static struct view RomView;
static struct view RegistersView;
static struct view FlagsView;
static struct view DatapathView;
static struct view RamView;

static void drawhwtraits(const struct control *appstate)
{
    int cursor_y = 0;
    mvwprintw(HwView.content, cursor_y++, 0, "FPS: %dhz", Fps);
    mvwprintw(HwView.content, cursor_y++, 0, "dT: %.3f (+%.3f)", FrameTimeMs,
              FrameLeftMs);
    mvwprintw(HwView.content, cursor_y++, 0, "CB: %d (%.3f)",
              appstate->cyclebudget, CycleBudgetMs);
    mvwaddstr(HwView.content, cursor_y++, 0, "Master Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "CPU Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "PPU Clock: INF Hz");
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles: %llu",
              appstate->total_cycles);
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles per Second: %d",
              appstate->cycles_per_sec);
    mvwaddstr(HwView.content, cursor_y++, 0, "Cycles per Frame: N/A");
}

static void drawtoggle(const char *label, bool state)
{
    if (state) {
        wattron(ControlsView.content, A_STANDOUT);
    }
    wprintw(ControlsView.content, " %s ", label);
    wattroff(ControlsView.content, A_STANDOUT);
    if (state) {
        wattroff(ControlsView.content, A_STANDOUT);
    }
}

static void drawcontrols(const struct control *appstate)
{
    int cursor_y = 0;
    wattron(ControlsView.content, A_STANDOUT);
    mvwaddstr(ControlsView.content, cursor_y, 0, " HALT ");
    wattroff(ControlsView.content, A_STANDOUT);

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Mode: ");
    drawtoggle("Cycle", appstate->exec_mode == EXC_CYCLE);
    drawtoggle("Step", appstate->exec_mode == EXC_STEP);
    drawtoggle("Run", appstate->exec_mode == EXC_RUN);

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Send: ");
    waddstr(ControlsView.content, " IRQ ");
    waddstr(ControlsView.content, " NMI ");
    waddstr(ControlsView.content, " RES ");
    mvwhline(ControlsView.content, ++cursor_y, 0, 0,
             getmaxx(ControlsView.content));

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y++, 0, "FPS [1-120]: N/A");
    mvwprintw(ControlsView.content, cursor_y++, 0, "CPS [1-100]: %d",
              appstate->cycles_per_sec);
    mvwhline(ControlsView.content, ++cursor_y, 0, 0,
             getmaxx(ControlsView.content));

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y++, 0, "RAM: Next Prev");
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "ROM: up down");
    mvwaddstr(ControlsView.content, ++cursor_y, 5, "pgup pgdn");
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "Go:  [$8000-$FFFF]");
}

static void drawvecs(int h, int w, int y,
                     const struct console_state *snapshot)
{
    mvwhline(RomView.content, h - y--, 0, 0, w);

    uint16_t vaddr = NmiVector & CpuCartAddrMask;
    uint8_t lo = snapshot->rom[vaddr],
            hi = snapshot->rom[vaddr + 1];
    mvwprintw(RomView.content, h - y--, 0, "$%04X: %02X %02X       NMI $%04X",
              NmiVector, lo, hi, bytowr(lo, hi));

    vaddr = ResetVector & CpuCartAddrMask;
    lo = snapshot->rom[vaddr];
    hi = snapshot->rom[vaddr + 1];
    mvwprintw(RomView.content, h - y--, 0, "$%04X: %02X %02X       RES $%04X",
              ResetVector, lo, hi, bytowr(lo, hi));

    vaddr = IrqVector & CpuCartAddrMask;
    lo = snapshot->rom[vaddr];
    hi = snapshot->rom[vaddr + 1];
    mvwprintw(RomView.content, h - y, 0, "$%04X: %02X %02X       IRQ $%04X",
              IrqVector, lo, hi, bytowr(lo, hi));
}

static void drawrom(const struct console_state *snapshot)
{
    static const int vector_offset = 4;

    int h, w;
    getmaxyx(RomView.content, h, w);
    werase(RomView.content);

    uint16_t addr = snapshot->cpu.currinst;
    // NOTE: on startup currinst is probably outside ROM range
    if (addr < CpuCartMinAddr) {
        mvwaddstr(RomView.content, 0, 0, "OUT OF ROM RANGE");
    } else {
        char disassembly[DIS_INST_SIZE];
        for (int i = 0, bytes = 0; i < h - vector_offset; ++i) {
            addr += bytes;
            const size_t cart_offset = addr & CpuCartAddrMask;
            bytes = dis_inst(addr, snapshot->rom + cart_offset,
                             ROM_SIZE - cart_offset, disassembly);
            if (bytes == 0) break;
            if (bytes < 0) {
                mvwaddstr(RomView.content, i, 0, dis_errstr(bytes));
                break;
            }
            mvwaddstr(RomView.content, i, 0, disassembly);
        }
    }

    drawvecs(h, w, vector_offset, snapshot);
}

static void drawregister(const struct console_state *snapshot)
{
    int cursor_y = 0;
    mvwprintw(RegistersView.content, cursor_y++, 0, "PC: $%04X",
              snapshot->cpu.program_counter);
    mvwprintw(RegistersView.content, cursor_y++, 0, "S:  $%02X",
              snapshot->cpu.stack_pointer);
    mvwhline(RegistersView.content, cursor_y++, 0, 0,
             getmaxx(RegistersView.content));
    mvwprintw(RegistersView.content, cursor_y++, 0, "A:  $%02X",
              snapshot->cpu.accumulator);
    mvwprintw(RegistersView.content, cursor_y++, 0, "X:  $%02X",
              snapshot->cpu.xindex);
    mvwprintw(RegistersView.content, cursor_y++, 0, "Y:  $%02X",
              snapshot->cpu.yindex);
}

static void drawflags(const struct console_state *snapshot)
{
    int cursor_x = 0, cursor_y = 0;
    mvwaddstr(FlagsView.content, cursor_y++, cursor_x, "7 6 5 4 3 2 1 0");
    mvwaddstr(FlagsView.content, cursor_y++, cursor_x, "N V - B D I Z C");
    mvwhline(FlagsView.content, cursor_y++, cursor_x, 0,
             getmaxx(FlagsView.content));
    for (size_t i = sizeof snapshot->cpu.status * 8; i > 0; --i) {
        mvwprintw(FlagsView.content, cursor_y, cursor_x, "%u",
                  (snapshot->cpu.status >> (i - 1)) & 1);
        cursor_x += 2;
    }
}

static void drawline(bool signal, int y, int x, int dir_offset,
                     const char *restrict direction,
                     const char *restrict label)
{
    if (!signal) {
        wattron(DatapathView.content, A_DIM);
    }
    mvwaddstr(DatapathView.content, y, x - 1, label);
    mvwaddstr(DatapathView.content, y + dir_offset, x, direction);
    if (!signal) {
        wattroff(DatapathView.content, A_DIM);
    }
}

static void drawdatapath(const struct console_state *snapshot)
{
    static const char *const restrict left = "\u2190",
                      *const restrict right = "\u2192",
                      *const restrict up = "\u2191",
                      *const restrict down = "\u2193";
    static const int vsep1 = 1, vsep2 = 9, vsep3 = 23, vsep4 = 29;
    // NOTE: static buffer to remember last datapath paint
    static char buf[DIS_DATAP_SIZE];

    const int w = getmaxx(DatapathView.content), line_x = (w / 4) + 1;
    int cursor_y = 0;
    werase(DatapathView.content);

    drawline(snapshot->lines.ready, cursor_y, line_x, 1, down, "RDY");
    drawline(snapshot->lines.sync, cursor_y, line_x * 2, 1, up, "SYNC");
    drawline(snapshot->lines.readwrite, cursor_y++, line_x * 3, 1, up, "R/W");

    mvwhline(DatapathView.content, ++cursor_y, 0, 0, w);

    mvwvline(DatapathView.content, ++cursor_y, vsep1, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep2, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep3, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep4, 0, 3);
    const int wlen = dis_datapath(snapshot, buf);
    const char *const mnemonic = wlen < 0 ? dis_errstr(wlen) : buf;
    mvwaddstr(DatapathView.content, cursor_y, vsep2 + 2, mnemonic);

    mvwaddstr(DatapathView.content, ++cursor_y, 0, left);
    mvwprintw(DatapathView.content, cursor_y, vsep1 + 2, "$%04X",
              snapshot->cpu.addressbus);
    const int dbus_x = vsep3 + 2;
    if (snapshot->cpu.datafault) {
        mvwaddstr(DatapathView.content, cursor_y, dbus_x, "FLT");
    } else {
        mvwprintw(DatapathView.content, cursor_y, dbus_x, "$%02X",
                  snapshot->cpu.databus);
    }
    mvwaddstr(DatapathView.content, cursor_y, vsep4 + 1,
              snapshot->lines.readwrite ? left : right);

    mvwprintw(DatapathView.content, ++cursor_y, vsep2 + 2, "%*sT%u",
              snapshot->cpu.exec_cycle, "", snapshot->cpu.exec_cycle);

    mvwhline(DatapathView.content, ++cursor_y, 0, 0, w);

    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    drawline(snapshot->lines.irq, cursor_y, line_x, -1, up,
             "I\u0305R\u0305Q\u0305");
    drawline(snapshot->lines.nmi, cursor_y, line_x * 2, -1, up,
             "N\u0305M\u0305I\u0305");
    drawline(snapshot->lines.reset, cursor_y, line_x * 3, -1, up,
             "R\u0305E\u0305S\u0305");
}

static void drawram(const struct console_state *snapshot)
{
    static const int start_x = 5, col_width = 3;

    int cursor_x = start_x, cursor_y = 0;
    mvwvline(RamView.content, 0, start_x - 2, 0, getmaxy(RamView.content));
    for (size_t page = 0; page < 8; ++page) {
        for (size_t page_row = 0; page_row < 0x10; ++page_row) {
            mvwprintw(RamView.content, cursor_y, 0, "%02X", page);
            for (size_t page_col = 0; page_col < 0x10; ++page_col) {
                mvwprintw(RamView.content, cursor_y, cursor_x, "%02X",
                          snapshot->ram[(page * 0x100)
                                        + (page_row * 0x10)
                                        + page_col]);
                cursor_x += col_width;
            }
            cursor_x = start_x;
            ++cursor_y;
        }
        if (page % 2 == 0) {
            ++cursor_y;
        }
    }
}

static void createwin(struct view *v, int h, int w, int y, int x,
                      const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->outer = new_panel(v->win);
    box(v->win, 0, 0);
    mvwaddstr(v->win, 0, 1, title);
}

static void vinit(struct view *v, int h, int w, int y, int x,
                  const char *restrict title)
{
    createwin(v, h, w, y, x, title);
    v->content = derwin(v->win, h - 4, w - 4, 2, 2);
    v->inner = new_panel(v->content);
}

static void raminit(int h, int w, int y, int x)
{
    createwin(&RamView, h, w, y, x, "RAM");
    RamView.content = newpad((h - 4) * RamSheets, w - 4);
}

static void vcleanup(struct view *v)
{
    del_panel(v->inner);
    delwin(v->content);
    del_panel(v->outer);
    delwin(v->win);
    *v = (struct view){0};
}

static void ramrefresh(int ramsheet)
{
    int ram_x, ram_y, ram_w, ram_h;
    getbegyx(RamView.win, ram_y, ram_x);
    getmaxyx(RamView.win, ram_h, ram_w);
    pnoutrefresh(RamView.content, (ram_h - 4) * ramsheet, 0, ram_y + 2,
                 ram_x + 2, ram_y + ram_h - 3, ram_x + ram_w - 2);
}

//
// Public Interface
//

void ui_init(void)
{
    static const int col1w = 32, col2w = 34, col3w = 35, col4w = 56, hwh = 13,
                     cpuh = 10, flagsh = 8, flagsw = 19, ramh = 37;

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    curs_set(0);

    int scrh, scrw;
    getmaxyx(stdscr, scrh, scrw);
    const int yoffset = (scrh - ramh) / 2,
              xoffset = (scrw - (col1w + col2w + col3w + col4w)) / 2;
    vinit(&HwView, hwh, col1w, yoffset, xoffset, "Hardware Traits");
    vinit(&ControlsView, ramh - hwh, col1w, yoffset + hwh, xoffset,
          "Controls");
    vinit(&RomView, ramh, col2w, yoffset, xoffset + col1w, "ROM");
    vinit(&RegistersView, cpuh, flagsw, yoffset, xoffset + col1w + col2w,
          "Registers");
    vinit(&FlagsView, flagsh, flagsw, yoffset + cpuh, xoffset + col1w + col2w,
          "Flags");
    vinit(&DatapathView, 13, col3w, yoffset + cpuh + flagsh,
          xoffset + col1w + col2w, "Datapath");
    raminit(ramh, col4w, yoffset, xoffset + col1w + col2w + col3w);

    initclock();
}

void ui_cleanup(void)
{
    vcleanup(&RamView);
    vcleanup(&DatapathView);
    vcleanup(&FlagsView);
    vcleanup(&RegistersView);
    vcleanup(&RomView);
    vcleanup(&ControlsView);
    vcleanup(&HwView);

    endwin();
}

void ui_start_tick(struct control *appstate)
{
    clock_gettime(CLOCK_MONOTONIC, &Current);
    CycleBudgetMs += FrameTimeMs = to_ms(&Current) - to_ms(&Previous);

    // Accumulate at most a second of banked cycle time
    if (CycleBudgetMs >= MillisecondsPerSecond) {
        CycleBudgetMs = MillisecondsPerSecond;
    }

    const double mspercycle = MillisecondsPerSecond / appstate->cycles_per_sec;
    if (CycleBudgetMs >= mspercycle) {
        const int new_cycles = CycleBudgetMs / mspercycle;
        appstate->cyclebudget += new_cycles;
        CycleBudgetMs -= new_cycles * mspercycle;
    }
}

void ui_end_tick(void)
{
    Previous = Current;
    sleeploop();
}

int ui_pollinput(void)
{
    return getch();
}

void ui_refresh(const struct control *appstate,
                const struct console_state *snapshot)
{
    drawhwtraits(appstate);
    drawcontrols(appstate);
    drawrom(snapshot);
    drawregister(snapshot);
    drawflags(snapshot);
    drawdatapath(snapshot);
    drawram(snapshot);

    update_panels();
    ramrefresh(appstate->ramsheet);
    doupdate();
}

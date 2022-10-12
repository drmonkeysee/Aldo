//
//  uincurses.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "ui.h"

#include "bytes.h"
#include "cart.h"
#include "debug.h"
#include "dis.h"
#include "haltexpr.h"
#include "tsutil.h"

#include <ncurses.h>
#include <panel.h>

#include <assert.h>
#include <inttypes.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

//
// Run Loop Clock
//

// NOTE: Approximate 60 FPS for application event loop;
// this will be enforced by actual vsync when ported to true GUI
// and is *distinct* from emulator frequency which can be modified by the user.
static const int Fps = 60;
static const struct timespec VSync = {.tv_nsec = TSU_NS_PER_S / Fps};

static struct timespec Current, Previous, Start;
static double FrameLeftMs, FrameTimeMs, TimeBudgetMs;

static void initclock(void)
{
    clock_gettime(CLOCK_MONOTONIC, &Start);
    Previous = Start;
}

static void tick_sleep(void)
{
    const struct timespec elapsed = timespec_elapsed(&Current);

    // NOTE: if elapsed nanoseconds is greater than vsync we're over
    // our time budget; if elapsed *seconds* is greater than vsync
    // we've BLOWN AWAY our time budget; either way don't sleep.
    if (elapsed.tv_nsec > VSync.tv_nsec || elapsed.tv_sec > VSync.tv_sec) {
        // NOTE: we've already blown the frame time so convert everything
        // to milliseconds to make the math easier.
        FrameLeftMs = timespec_to_ms(&VSync) - timespec_to_ms(&elapsed);
        return;
    }

    const struct timespec tick_left = {
        .tv_nsec = VSync.tv_nsec - elapsed.tv_nsec,
    };
    FrameLeftMs = timespec_to_ms(&tick_left);

    timespec_sleep(tick_left);
}

//
// UI Widgets
//

static struct view {
    WINDOW *restrict win, *restrict content;
    PANEL *restrict outer, *restrict inner;
}
    HwView,
    ControlsView,
    DebuggerView,
    CartView,
    PrgView,
    RegistersView,
    FlagsView,
    DatapathView,
    RamView;

static void drawhwtraits(const struct control *appstate)
{
    // NOTE: update timing metrics on a readable interval
    static const double refresh_interval_ms = 250;
    static double display_frameleft, display_frametime, refreshdt;
    if ((refreshdt += FrameTimeMs) >= refresh_interval_ms) {
        display_frameleft = FrameLeftMs;
        display_frametime = FrameTimeMs;
        refreshdt = 0;
    }

    int cursor_y = 0;
    werase(HwView.content);
    mvwprintw(HwView.content, cursor_y++, 0, "FPS: %d (%.2f)", Fps,
              appstate->clock.frames / appstate->clock.runtime);
    mvwprintw(HwView.content, cursor_y++, 0, "\u0394T: %.3f (%+.3f)",
              display_frametime, display_frameleft);
    mvwprintw(HwView.content, cursor_y++, 0, "Frames: %" PRIu64,
              appstate->clock.frames);
    mvwprintw(HwView.content, cursor_y++, 0, "Runtime: %.3f",
              appstate->clock.runtime);
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles: %" PRIu64,
              appstate->clock.total_cycles);
    mvwaddstr(HwView.content, cursor_y++, 0, "Master Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "CPU/PPU Clock: INF/INF Hz");
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles per Second: %d",
              appstate->clock.cycles_per_sec);
    mvwaddstr(HwView.content, cursor_y++, 0, "Cycles per Frame: N/A");
    mvwprintw(HwView.content, cursor_y, 0, "BCD Supported: %s",
              appstate->bcdsupport ? "Yes" : "No");
}

static void drawtoggle(const char *label, bool selected)
{
    if (selected) {
        wattron(ControlsView.content, A_STANDOUT);
    }
    waddstr(ControlsView.content, label);
    if (selected) {
        wattroff(ControlsView.content, A_STANDOUT);
    }
}

static void drawcontrols(const struct console_state *snapshot)
{
    static const char *const restrict halt = "HALT";

    const int w = getmaxx(ControlsView.content);
    int cursor_y = 0;
    werase(ControlsView.content);
    wmove(ControlsView.content, cursor_y, 0);

    const double center_offset = (w - strlen(halt)) / 2.0;
    assert(center_offset > 0);
    char halt_label[w + 1];
    snprintf(halt_label, sizeof halt_label, "%*s%s%*s",
             (int)round(center_offset), "", halt,
             (int)floor(center_offset), "");
    drawtoggle(halt_label, !snapshot->lines.ready);

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Mode: ");
    drawtoggle(" Cycle ", snapshot->mode == NEXC_CYCLE);
    drawtoggle(" Step ", snapshot->mode == NEXC_STEP);
    drawtoggle(" Run ", snapshot->mode == NEXC_RUN);

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Signal: ");
    drawtoggle(" IRQ ", !snapshot->lines.irq);
    drawtoggle(" NMI ", !snapshot->lines.nmi);
    drawtoggle(" RES ", !snapshot->lines.reset);

    mvwhline(ControlsView.content, ++cursor_y, 0, 0, w);
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "Halt/Run: <Space>");
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "Mode: m/M");
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "Signal: i, n, s");
    mvwaddstr(ControlsView.content, ++cursor_y, 0,
              "Speed \u00b11 (\u00b110): -/= (_/+)");
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "Ram F/B: r/R");
    mvwaddstr(ControlsView.content, ++cursor_y, 0, "Quit: q");
}

static void drawdebugger(const struct control *appstate,
                         const struct console_state *snapshot)
{
    int cursor_y = 0;
    werase(DebuggerView.content);
    mvwprintw(DebuggerView.content, cursor_y++, 0, "Tracing: %s",
              appstate->tron ? "On" : "Off");
    mvwaddstr(DebuggerView.content, cursor_y++, 0, "Reset Override: ");
    if (snapshot->debugger.resvector_override >= 0) {
        wprintw(DebuggerView.content, "$%04X",
                snapshot->debugger.resvector_override);
    } else {
        waddstr(DebuggerView.content, "None");
    }
    char break_desc[HEXPR_FMT_SIZE];
    const int err = haltexpr_fmt(&snapshot->debugger.break_condition,
                                 break_desc);
    mvwprintw(DebuggerView.content, cursor_y, 0, "Break: %s",
              err < 0 ? haltexpr_errstr(err) : break_desc);
}

static void drawcart(const struct control *appstate,
                     const struct console_state *snapshot)
{
    static const char *const restrict namelabel = "Name: ";

    const int maxwidth = getmaxx(CartView.content) - strlen(namelabel);
    int cursor_y = 0;
    mvwaddstr(CartView.content, cursor_y, 0, namelabel);
    const char
        *const cn = ctrl_cartfilename(appstate->cartfile),
        *endofname = strrchr(cn, '.');
    if (!endofname) {
        endofname = strrchr(cn, '\0');
    }
    const ptrdiff_t namelen = endofname - cn;
    const bool longname = namelen > maxwidth;
    // NOTE: ellipsis is one glyph wide despite being > 1 byte long
    wprintw(CartView.content, "%.*s%s",
            (int)(longname ? maxwidth - 1 : namelen), cn,
            longname ? "\u2026" : "");
    char fmtd[CART_FMT_SIZE];
    const int result = cart_format_extendedname(snapshot->cart.info, fmtd);
    mvwprintw(CartView.content, ++cursor_y, 0, "Format: %s",
              result > 0 ? fmtd : "Invalid Format");
}

static void drawinstructions(uint16_t addr, int h, int y,
                             const struct console_state *snapshot)
{
    struct dis_instruction inst = {0};
    for (int i = 0; i < h - y; ++i) {
        char disassembly[DIS_INST_SIZE];
        int result = dis_parsemem_inst(snapshot->mem.prglength,
                                       snapshot->mem.currprg,
                                       inst.offset + inst.bv.size,
                                       &inst);
        if (result > 0) {
            result = dis_inst(addr, &inst, disassembly);
        } else {
            if (result < 0) {
                mvwaddstr(PrgView.content, i, 0, dis_errstr(result));
            }
            break;
        }
        mvwaddstr(PrgView.content, i, 0, disassembly);
        addr += inst.bv.size;
    }
}

static void drawvecs(int h, int w, int y, const struct console_state *snapshot)
{
    mvwhline(PrgView.content, h - y--, 0, 0, w);

    uint8_t lo = snapshot->mem.vectors[0],
            hi = snapshot->mem.vectors[1];
    mvwprintw(PrgView.content, h - y--, 0, "%04X: %02X %02X     NMI $%04X",
              CPU_VECTOR_NMI, lo, hi, bytowr(lo, hi));

    lo = snapshot->mem.vectors[2];
    hi = snapshot->mem.vectors[3];
    mvwprintw(PrgView.content, h - y--, 0, "%04X: %02X %02X     RES",
              CPU_VECTOR_RES, lo, hi);
    if (snapshot->debugger.resvector_override >= 0) {
        wprintw(PrgView.content, " !$%04X",
                snapshot->debugger.resvector_override);
    } else {
        wprintw(PrgView.content, " $%04X", bytowr(lo, hi));
    }

    lo = snapshot->mem.vectors[4];
    hi = snapshot->mem.vectors[5];
    mvwprintw(PrgView.content, h - y, 0, "%04X: %02X %02X     IRQ $%04X",
              CPU_VECTOR_IRQ, lo, hi, bytowr(lo, hi));
}

static void drawprg(const struct console_state *snapshot)
{
    static const int vector_offset = 4;

    int h, w;
    getmaxyx(PrgView.content, h, w);
    werase(PrgView.content);

    drawinstructions(snapshot->datapath.current_instruction, h, vector_offset,
                     snapshot);
    drawvecs(h, w, vector_offset, snapshot);
}

static void drawregister(const struct console_state *snapshot)
{
    int cursor_y = 0;
    mvwprintw(RegistersView.content, cursor_y++, 0, "PC: %04X",
              snapshot->cpu.program_counter);
    mvwprintw(RegistersView.content, cursor_y++, 0, "S:  %02X",
              snapshot->cpu.stack_pointer);
    mvwprintw(RegistersView.content, cursor_y++, 0, "P:  %02X",
              snapshot->cpu.status);
    mvwhline(RegistersView.content, cursor_y++, 0, 0,
             getmaxx(RegistersView.content));
    mvwprintw(RegistersView.content, cursor_y++, 0, "A:  %02X",
              snapshot->cpu.accumulator);
    mvwprintw(RegistersView.content, cursor_y++, 0, "X:  %02X",
              snapshot->cpu.xindex);
    mvwprintw(RegistersView.content, cursor_y, 0, "Y:  %02X",
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

static void draw_cpu_line(bool signal, int y, int x, int dir_offset,
                          const char *direction, const char *label)
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

static void draw_interrupt_latch(enum nistate interrupt, int y, int x)
{
    const char *modifier = "";
    attr_t style = A_NORMAL;
    switch (interrupt) {
    case NIS_CLEAR:
        // NOTE: draw nothing for CLEAR state
        return;
    case NIS_DETECTED:
        style = A_DIM;
        break;
    case NIS_COMMITTED:
        style = A_UNDERLINE;
        modifier = "\u0305";
        break;
    case NIS_SERVICED:
        modifier = "\u0338";
        break;
    default:
        break;
    }

    if (style != A_NORMAL) {
        wattron(DatapathView.content, style);
    }
    mvwprintw(DatapathView.content, y, x, "%s%s", "\u25ef", modifier);
    if (style != A_NORMAL) {
        wattroff(DatapathView.content, style);
    }
}

static void drawdatapath(const struct console_state *snapshot)
{
    static const char
        *const restrict left = "\u2190",
        *const restrict right = "\u2192",
        *const restrict up = "\u2191",
        *const restrict down = "\u2193";
    static const int vsep1 = 1, vsep2 = 8, vsep3 = 22, vsep4 = 27, seph = 5;

    const int w = getmaxx(DatapathView.content), line_x = w / 4;
    int cursor_y = 0;
    werase(DatapathView.content);

    draw_cpu_line(snapshot->lines.ready, cursor_y, line_x, 1, down, "RDY");
    draw_cpu_line(snapshot->lines.sync, cursor_y, line_x * 2, 1, up, "SYNC");
    draw_cpu_line(snapshot->lines.readwrite, cursor_y++, line_x * 3, 1, up,
                  "R/W\u0305");

    mvwhline(DatapathView.content, ++cursor_y, 0, 0, w);

    mvwvline(DatapathView.content, ++cursor_y, vsep1, 0, seph);
    mvwvline(DatapathView.content, cursor_y, vsep2, 0, seph);
    mvwvline(DatapathView.content, cursor_y, vsep3, 0, seph);
    mvwvline(DatapathView.content, cursor_y, vsep4, 0, seph);

    if (snapshot->datapath.jammed) {
        wattron(DatapathView.content, A_STANDOUT);
        mvwaddstr(DatapathView.content, cursor_y, vsep2 + 2, " JAMMED ");
        wattroff(DatapathView.content, A_STANDOUT);
    } else {
        char buf[DIS_DATAP_SIZE];
        const int wlen = dis_datapath(snapshot, buf);
        const char *const mnemonic = wlen < 0 ? dis_errstr(wlen) : buf;
        mvwaddstr(DatapathView.content, cursor_y, vsep2 + 2, mnemonic);
    }

    mvwprintw(DatapathView.content, ++cursor_y, vsep2 + 2, "adl: %02X",
              snapshot->datapath.addrlow_latch);

    mvwaddstr(DatapathView.content, ++cursor_y, 0, left);
    mvwprintw(DatapathView.content, cursor_y, vsep1 + 2, "%04X",
              snapshot->datapath.addressbus);
    mvwprintw(DatapathView.content, cursor_y, vsep2 + 2, "adh: %02X",
              snapshot->datapath.addrhigh_latch);
    const int dbus_x = vsep3 + 2;
    if (snapshot->datapath.busfault) {
        mvwaddstr(DatapathView.content, cursor_y, dbus_x, "FLT");
    } else {
        mvwprintw(DatapathView.content, cursor_y, dbus_x, "%02X",
                  snapshot->datapath.databus);
    }
    mvwaddstr(DatapathView.content, cursor_y, vsep4 + 1,
              snapshot->lines.readwrite ? left : right);

    mvwprintw(DatapathView.content, ++cursor_y, vsep2 + 2, "adc: %02X",
              snapshot->datapath.addrcarry_latch);

    mvwprintw(DatapathView.content, ++cursor_y, vsep2 + 2, "%*sT%u",
              snapshot->datapath.exec_cycle, "",
              snapshot->datapath.exec_cycle);

    mvwhline(DatapathView.content, ++cursor_y, 0, 0, w);

    draw_interrupt_latch(snapshot->datapath.irq, cursor_y, line_x);
    draw_interrupt_latch(snapshot->datapath.nmi, cursor_y, line_x * 2);
    draw_interrupt_latch(snapshot->datapath.res, cursor_y, line_x * 3);
    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    draw_cpu_line(snapshot->lines.irq, cursor_y, line_x, -1, up,
                  "I\u0305R\u0305Q\u0305");
    draw_cpu_line(snapshot->lines.nmi, cursor_y, line_x * 2, -1, up,
                  "N\u0305M\u0305I\u0305");
    draw_cpu_line(snapshot->lines.reset, cursor_y, line_x * 3, -1, up,
                  "R\u0305E\u0305S\u0305");
}

static void drawram(const struct console_state *snapshot)
{
    static const int
        start_x = 5, col_width = 3, toprail_start = start_x + col_width;

    for (int col = 0; col < 0x10; ++col) {
        mvwprintw(RamView.win, 1, toprail_start + (col * col_width), "%X",
                  col);
    }
    mvwhline(RamView.win, 2, toprail_start - 1, 0,
             getmaxx(RamView.win) - toprail_start - 5);

    const int h = getmaxy(RamView.content);
    int cursor_x = start_x, cursor_y = 0;
    mvwvline(RamView.content, 0, start_x - 2, 0, h);
    mvwvline(RamView.content, 0, getmaxx(RamView.content) - 3, 0, h);
    for (size_t page = 0; page < 8; ++page) {
        for (size_t page_row = 0; page_row < 0x10; ++page_row) {
            mvwprintw(RamView.content, cursor_y, 0, "%02zX", page);
            for (size_t page_col = 0; page_col < 0x10; ++page_col) {
                const size_t ramidx = (page * 0x100) + (page_row * 0x10)
                                        + page_col;
                const bool sp = page == 1 && ramidx % 0x100
                                                == snapshot->cpu.stack_pointer;
                if (sp) {
                    wattron(RamView.content, A_STANDOUT);
                }
                mvwprintw(RamView.content, cursor_y, cursor_x, "%02X",
                          snapshot->mem.ram[ramidx]);
                if (sp) {
                    wattroff(RamView.content, A_STANDOUT);
                }
                cursor_x += col_width;
            }
            mvwprintw(RamView.content, cursor_y, cursor_x + 2,
                      "%zX", page_row);
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

static void vinit(struct view *v, int h, int w, int y, int x, int vpad,
                  const char *restrict title)
{
    createwin(v, h, w, y, x, title);
    v->content = derwin(v->win, h - (vpad * 2), w - 4, vpad, 2);
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
    pnoutrefresh(RamView.content, (ram_h - 4) * ramsheet, 0, ram_y + 3,
                 ram_x + 2, ram_y + ram_h - 2, ram_x + ram_w - 2);
}

//
// UI Interface Implementation
//

static void ncurses_init(void)
{
    static const int
        col1w = 32, col2w = 31, col3w = 33, col4w = 60, hwh = 14, ctrlh = 16,
        crth = 6, cpuh = 11, flagsh = 8, flagsw = 19, ramh = 37;

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    curs_set(0);

    int scrh, scrw;
    getmaxyx(stdscr, scrh, scrw);
    const int
        yoffset = (scrh - ramh) / 2,
        xoffset = (scrw - (col1w + col2w + col3w + col4w)) / 2;
    vinit(&HwView, hwh, col1w, yoffset, xoffset, 2, "Hardware Traits");
    vinit(&ControlsView, ctrlh, col1w, yoffset + hwh, xoffset, 2, "Controls");
    vinit(&DebuggerView, 7, col1w, yoffset + hwh + ctrlh, xoffset, 2,
          "Debugger");
    vinit(&CartView, crth, col2w, yoffset, xoffset + col1w, 2, "Cart");
    vinit(&PrgView, ramh - crth, col2w, yoffset + crth, xoffset + col1w, 1,
          "PRG");
    vinit(&RegistersView, cpuh, flagsw, yoffset, xoffset + col1w + col2w, 2,
          "Registers");
    vinit(&FlagsView, flagsh, flagsw, yoffset + cpuh, xoffset + col1w + col2w,
          2, "Flags");
    vinit(&DatapathView, 13, col3w, yoffset + cpuh + flagsh,
          xoffset + col1w + col2w, 1, "Datapath");
    raminit(ramh, col4w, yoffset, xoffset + col1w + col2w + col3w);

    initclock();
}

static void ncurses_tick_start(struct control *appstate,
                               const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    clock_gettime(CLOCK_MONOTONIC, &Current);
    const double currentms = timespec_to_ms(&Current);
    FrameTimeMs = currentms - timespec_to_ms(&Previous);
    appstate->clock.runtime = (currentms - timespec_to_ms(&Start))
                                / TSU_MS_PER_S;

    if (!snapshot->lines.ready) {
        TimeBudgetMs = appstate->clock.budget = 0;
        return;
    }

    TimeBudgetMs += FrameTimeMs;
    // NOTE: accumulate at most a second of banked cycle time
    if (TimeBudgetMs >= TSU_MS_PER_S) {
        TimeBudgetMs = TSU_MS_PER_S;
    }

    const double mspercycle = (double)TSU_MS_PER_S
                                / appstate->clock.cycles_per_sec;
    const int new_cycles = TimeBudgetMs / mspercycle;
    appstate->clock.budget += new_cycles;
    TimeBudgetMs -= new_cycles * mspercycle;
}

static void ncurses_tick_end(struct control *appstate)
{
    Previous = Current;
    ++appstate->clock.frames;
    tick_sleep();
}

static int ncurses_pollinput(void)
{
    return getch();
}

static void ncurses_refresh(const struct control *appstate,
                            const struct console_state *snapshot)
{
    assert(appstate != NULL);
    assert(snapshot != NULL);

    drawhwtraits(appstate);
    drawcontrols(snapshot);
    drawdebugger(appstate, snapshot);
    drawcart(appstate, snapshot);
    drawprg(snapshot);
    drawregister(snapshot);
    drawflags(snapshot);
    drawdatapath(snapshot);
    drawram(snapshot);

    update_panels();
    ramrefresh(appstate->ramsheet);
    doupdate();
}

static void ncurses_cleanup(const struct control *appstate,
                            const struct console_state *snapshot)
{
    (void)appstate, (void)snapshot;

    vcleanup(&RamView);
    vcleanup(&DatapathView);
    vcleanup(&FlagsView);
    vcleanup(&RegistersView);
    vcleanup(&PrgView);
    vcleanup(&CartView);
    vcleanup(&DebuggerView);
    vcleanup(&ControlsView);
    vcleanup(&HwView);

    endwin();
}

//
// Public Interface
//

int ui_ncurses_init(struct ui_interface *ui)
{
    assert(ui != NULL);

    ncurses_init();
    *ui = (struct ui_interface){
        ncurses_tick_start,
        ncurses_tick_end,
        ncurses_pollinput,
        ncurses_refresh,
        ncurses_cleanup,
    };
    return 0;
}

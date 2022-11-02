//
//  uicurses.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "bytes.h"
#include "cart.h"
#include "cliargs.h"
#include "ctrlsignal.h"
#include "cycleclock.h"
#include "dis.h"
#include "haltexpr.h"
#include "nes.h"
#include "snapshot.h"
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
static const int Fps = 60, MinCps = 1, MaxCps = 1000, RamSheets = 4;
static const struct timespec VSync = {.tv_nsec = TSU_NS_PER_S / Fps};

static struct {
    struct timespec current, previous, start;
    struct cycleclock cyclock;
    double frameleft_ms, frametime_ms, timebudget_ms;
} Clock = {.cyclock = {.cycles_per_sec = 4}};
static struct {
    int ramsheet;
    bool bcdsupport, running, tron;
} ViewState = {.running = true};

static void initclock(void)
{
    clock_gettime(CLOCK_MONOTONIC, &Clock.start);
    Clock.previous = Clock.start;
}

static void tick_sleep(void)
{
    const struct timespec elapsed = timespec_elapsed(&Clock.current);

    // NOTE: if elapsed nanoseconds is greater than vsync we're over
    // our time budget; if elapsed *seconds* is greater than vsync
    // we've BLOWN AWAY our time budget; either way don't sleep.
    if (elapsed.tv_nsec > VSync.tv_nsec || elapsed.tv_sec > VSync.tv_sec) {
        // NOTE: we've already blown the frame time so convert everything
        // to milliseconds to make the math easier.
        Clock.frameleft_ms = timespec_to_ms(&VSync) - timespec_to_ms(&elapsed);
        return;
    }

    const struct timespec tick_left = {
        .tv_nsec = VSync.tv_nsec - elapsed.tv_nsec,
    };
    Clock.frameleft_ms = timespec_to_ms(&tick_left);

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

static void drawhwtraits(void)
{
    // NOTE: update timing metrics on a readable interval
    static const double refresh_interval_ms = 250;
    static double display_frameleft, display_frametime, refreshdt;
    if ((refreshdt += Clock.frametime_ms) >= refresh_interval_ms) {
        display_frameleft = Clock.frameleft_ms;
        display_frametime = Clock.frametime_ms;
        refreshdt = 0;
    }

    int cursor_y = 0;
    werase(HwView.content);
    mvwprintw(HwView.content, cursor_y++, 0, "FPS: %d (%.2f)", Fps,
              (double)Clock.cyclock.frames / Clock.cyclock.runtime);
    mvwprintw(HwView.content, cursor_y++, 0, "\u0394T: %.3f (%+.3f)",
              display_frametime, display_frameleft);
    mvwprintw(HwView.content, cursor_y++, 0, "Frames: %" PRIu64,
              Clock.cyclock.frames);
    mvwprintw(HwView.content, cursor_y++, 0, "Runtime: %.3f",
              Clock.cyclock.runtime);
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles: %" PRIu64,
              Clock.cyclock.total_cycles);
    mvwaddstr(HwView.content, cursor_y++, 0, "Master Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "CPU/PPU Clock: INF/INF Hz");
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles per Second: %d",
              Clock.cyclock.cycles_per_sec);
    mvwaddstr(HwView.content, cursor_y++, 0, "Cycles per Frame: N/A");
    mvwprintw(HwView.content, cursor_y, 0, "BCD Supported: %s",
              ViewState.bcdsupport ? "Yes" : "No");
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

    const double center_offset = (w - (int)strlen(halt)) / 2.0;
    assert(center_offset > 0);
    char halt_label[w + 1];
    snprintf(halt_label, sizeof halt_label, "%*s%s%*s",
             (int)round(center_offset), "", halt,
             (int)floor(center_offset), "");
    drawtoggle(halt_label, !snapshot->lines.ready);

    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Mode: ");
    drawtoggle(" Cycle ", snapshot->mode == CSGM_CYCLE);
    drawtoggle(" Step ", snapshot->mode == CSGM_STEP);
    drawtoggle(" Run ", snapshot->mode == CSGM_RUN);

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

static void drawdebugger(const struct console_state *snapshot)
{
    int cursor_y = 0;
    werase(DebuggerView.content);
    mvwprintw(DebuggerView.content, cursor_y++, 0, "Tracing: %s",
              ViewState.tron ? "On" : "Off");
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

static void drawcart(const struct console_state *snapshot)
{
    static const char *const restrict namelabel = "Name: ";

    const int maxwidth = getmaxx(CartView.content) - (int)strlen(namelabel);
    int cursor_y = 0;
    mvwaddstr(CartView.content, cursor_y, 0, namelabel);
    const char
        *const cn = cart_filename(snapshot->cart.info),
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
    const int result = cart_format_extname(snapshot->cart.info, fmtd);
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
        addr += (uint16_t)inst.bv.size;
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

static void draw_interrupt_latch(enum csig_state interrupt, int y, int x)
{
    const char *modifier = "";
    attr_t style = A_NORMAL;
    switch (interrupt) {
    case CSGS_CLEAR:
        // NOTE: draw nothing for CLEAR state
        return;
    case CSGS_DETECTED:
        style = A_DIM;
        break;
    case CSGS_COMMITTED:
        style = A_UNDERLINE;
        modifier = "\u0305";
        break;
    case CSGS_SERVICED:
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

static void ramrefresh(void)
{
    int ram_x, ram_y, ram_w, ram_h;
    getbegyx(RamView.win, ram_y, ram_x);
    getmaxyx(RamView.win, ram_h, ram_w);
    pnoutrefresh(RamView.content, (ram_h - 4) * ViewState.ramsheet, 0,
                 ram_y + 3, ram_x + 2, ram_y + ram_h - 2, ram_x + ram_w - 2);
}

//
// UI Loop Implementation
//

static void init_ui(const struct cliargs *args)
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

    ViewState.bcdsupport = args->bcdsupport;
    ViewState.tron = args->tron;
    initclock();
}

static void tick_start(const struct console_state *snapshot)
{
    clock_gettime(CLOCK_MONOTONIC, &Clock.current);
    const double currentms = timespec_to_ms(&Clock.current);
    Clock.frametime_ms = currentms - timespec_to_ms(&Clock.previous);
    Clock.cyclock.runtime = (currentms - timespec_to_ms(&Clock.start))
                                / TSU_MS_PER_S;

    if (!snapshot->lines.ready) {
        Clock.timebudget_ms = Clock.cyclock.budget = 0;
        return;
    }

    Clock.timebudget_ms += Clock.frametime_ms;
    // NOTE: accumulate at most a second of banked cycle time
    if (Clock.timebudget_ms >= TSU_MS_PER_S) {
        Clock.timebudget_ms = TSU_MS_PER_S;
    }

    const double mspercycle = (double)TSU_MS_PER_S
                                / Clock.cyclock.cycles_per_sec;
    const int new_cycles = (int)(Clock.timebudget_ms / mspercycle);
    Clock.cyclock.budget += new_cycles;
    Clock.timebudget_ms -= new_cycles * mspercycle;
}

static void tick_end(void)
{
    Clock.previous = Clock.current;
    ++Clock.cyclock.frames;
    tick_sleep();
}

static void handle_input(nes *console, const struct console_state *snapshot)
{
    const int c = getch();
    switch (c) {
    case ' ':
        if (snapshot->lines.ready) {
            nes_halt(console);
        } else {
            nes_ready(console);
        }
        break;
    case '=':   // "Lowercase" +
        ++Clock.cyclock.cycles_per_sec;
        goto pclamp_cps;
    case '+':
        Clock.cyclock.cycles_per_sec += 10;
    pclamp_cps:
        if (Clock.cyclock.cycles_per_sec > MaxCps) {
            Clock.cyclock.cycles_per_sec = MaxCps;
        }
        break;
    case '-':
        --Clock.cyclock.cycles_per_sec;
        goto nclamp_cps;
    case '_':   // "Uppercase" -
        Clock.cyclock.cycles_per_sec -= 10;
    nclamp_cps:
        if (Clock.cyclock.cycles_per_sec < MinCps) {
            Clock.cyclock.cycles_per_sec = MinCps;
        }
        break;
    case 'i':
        if (snapshot->lines.irq) {
            nes_interrupt(console, CSGI_IRQ);
        } else {
            nes_clear(console, CSGI_IRQ);
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
            nes_interrupt(console, CSGI_NMI);
        } else {
            nes_clear(console, CSGI_NMI);
        }
        break;
    case 'q':
        ViewState.running = false;
        break;
    case 'r':
        ViewState.ramsheet = (ViewState.ramsheet + 1) % RamSheets;
        break;
    case 'R':
        --ViewState.ramsheet;
        if (ViewState.ramsheet < 0) {
            ViewState.ramsheet = RamSheets - 1;
        }
        break;
    case 's':
        if (snapshot->lines.reset) {
            nes_interrupt(console, CSGI_RES);
        } else {
            nes_clear(console, CSGI_RES);
        }
        break;
    }
}

static void refresh_ui(const struct console_state *snapshot)
{
    drawhwtraits();
    drawcontrols(snapshot);
    drawdebugger(snapshot);
    drawcart(snapshot);
    drawprg(snapshot);
    drawregister(snapshot);
    drawflags(snapshot);
    drawdatapath(snapshot);
    drawram(snapshot);

    update_panels();
    ramrefresh();
    doupdate();
}

static void cleanup_ui(void)
{
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

int ui_curses_loop(const struct cliargs *args, nes *console,
                   struct console_state *snapshot)
{
    assert(args != NULL);
    assert(console != NULL);
    assert(snapshot != NULL);

    init_ui(args);
    do {
        tick_start(snapshot);
        handle_input(console, snapshot);
        if (ViewState.running) {
            nes_cycle(console, &Clock.cyclock);
            nes_snapshot(console, snapshot);
            refresh_ui(snapshot);
        }
        tick_end();
    } while (ViewState.running);
    cleanup_ui();

    return 0;
}

const char *ui_curses_version(void)
{
    return curses_version();
}

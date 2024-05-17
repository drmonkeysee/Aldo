//
//  uicurses.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "argparse.h"
#include "bytes.h"
#include "cart.h"
#include "ctrlsignal.h"
#include "cycleclock.h"
#include "debug.h"
#include "dis.h"
#include "emu.h"
#include "haltexpr.h"
#include "nes.h"
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
static const int
    Fps = 60, RamColWidth = 3, RamDim = 16, RamPageSize = RamDim * RamDim,
    RamSheetSize = RamPageSize * 2;

struct viewstate {
    struct runclock {
        struct cycleclock cyclock;
        double frameleft_ms;
    } clock;
    int ramsheet, total_ramsheets;
    bool running;
};

static void tick_sleep(struct runclock *c)
{
    static const struct timespec vsync = {.tv_nsec = TSU_NS_PER_S / Fps};

    const struct timespec elapsed = timespec_elapsed(&c->cyclock.current);

    // NOTE: if elapsed nanoseconds is greater than vsync we're over
    // our time budget; if elapsed *seconds* is greater than vsync
    // we've BLOWN AWAY our time budget; either way don't sleep.
    if (elapsed.tv_nsec > vsync.tv_nsec || elapsed.tv_sec > vsync.tv_sec) {
        // NOTE: we've already blown the frame time so convert everything
        // to milliseconds to make the math easier.
        c->frameleft_ms = timespec_to_ms(&vsync) - timespec_to_ms(&elapsed);
        return;
    }

    const struct timespec tick_left = {
        .tv_nsec = vsync.tv_nsec - elapsed.tv_nsec,
    };
    c->frameleft_ms = timespec_to_ms(&tick_left);

    timespec_sleep(tick_left);
}

//
// UI Widgets
//

struct layout {
    struct view {
        WINDOW *win, *content;
        PANEL *outer, *inner;
    }
        system,
        debugger,
        cart,
        prg,
        cpu,
        ppu,
        ram;
};

static void drawstats(const struct view *v, const struct viewstate *vs,
                      const struct emulator *emu, int *cursor_y)
{
    // NOTE: update timing metrics on a readable interval
    static const double refresh_interval_ms = 250;
    static double display_frameleft, display_frametime, refreshdt;
    if ((refreshdt += vs->clock.cyclock.frametime_ms) >= refresh_interval_ms) {
        display_frameleft = vs->clock.frameleft_ms;
        display_frametime = vs->clock.cyclock.frametime_ms;
        refreshdt = 0;
    }

    mvwprintw(v->content, (*cursor_y)++, 0, "FPS: %d (%.2f)", Fps,
              (double)vs->clock.cyclock.frames / vs->clock.cyclock.runtime);
    mvwprintw(v->content, (*cursor_y)++, 0, "\u0394T: %.3f (%+.3f)",
              display_frametime, display_frameleft);
    mvwprintw(v->content, (*cursor_y)++, 0, "Frames: %" PRIu64,
              vs->clock.cyclock.frames);
    mvwprintw(v->content, (*cursor_y)++, 0, "Emutime: %.3f",
              vs->clock.cyclock.emutime);
    mvwprintw(v->content, (*cursor_y)++, 0, "Runtime: %.3f",
              vs->clock.cyclock.runtime);
    mvwprintw(v->content, (*cursor_y)++, 0, "Cycles: %" PRIu64,
              vs->clock.cyclock.total_cycles);
    mvwprintw(v->content, (*cursor_y)++, 0, "Cycles per Second: %d",
              vs->clock.cyclock.cycles_per_sec);
    mvwprintw(v->content, (*cursor_y)++, 0, "BCD Supported: %s",
              emu->args->bcdsupport ? "Yes" : "No");
}

static void drawtoggle(const struct view *v, const char *label, bool selected)
{
    if (selected) {
        wattron(v->content, A_STANDOUT);
    }
    waddstr(v->content, label);
    if (selected) {
        wattroff(v->content, A_STANDOUT);
    }
}

static void drawcontrols(const struct view *v, const struct emulator *emu,
                         int w, int cursor_y)
{
    static const char *const restrict halt = "HALT";

    wmove(v->content, cursor_y, 0);

    const double center_offset = (w - (int)strlen(halt)) / 2.0;
    assert(center_offset > 0);
    char halt_label[w + 1];
    snprintf(halt_label, sizeof halt_label, "%*s%s%*s",
             (int)round(center_offset), "", halt,
             (int)floor(center_offset), "");
    drawtoggle(v, halt_label, !emu->snapshot.lines.ready);

    cursor_y += 2;
    const enum csig_excmode mode = nes_mode(emu->console);
    mvwaddstr(v->content, cursor_y, 0, "Mode: ");
    drawtoggle(v, " Cycle ", mode == CSGM_CYCLE);
    drawtoggle(v, " Step ", mode == CSGM_STEP);
    drawtoggle(v, " Run ", mode == CSGM_RUN);

    cursor_y += 2;
    mvwaddstr(v->content, cursor_y, 0, "Signal: ");
    drawtoggle(v, " IRQ ", !emu->snapshot.lines.irq);
    drawtoggle(v, " NMI ", !emu->snapshot.lines.nmi);
    drawtoggle(v, " RES ", !emu->snapshot.lines.reset);

    mvwhline(v->content, ++cursor_y, 0, 0, w);
    mvwaddstr(v->content, ++cursor_y, 0, "Halt/Run: <Space>");
    mvwaddstr(v->content, ++cursor_y, 0, "Mode: m/M");
    mvwaddstr(v->content, ++cursor_y, 0, "Signal: i, n, s");
    mvwaddstr(v->content, ++cursor_y, 0,
              "Speed \u00b11 (\u00b110): -/= (_/+)");
    mvwaddstr(v->content, ++cursor_y, 0, "Ram F/B: r/R");
    mvwaddstr(v->content, ++cursor_y, 0, "Quit: q");
}

static void drawsystem(const struct view *v, const struct viewstate *vs,
                       const struct emulator *emu)
{
    const int w = getmaxx(v->content);
    int cursor_y = 0;
    werase(v->content);
    drawstats(v, vs, emu, &cursor_y);
    mvwhline(v->content, cursor_y++, 0, 0, w);
    drawcontrols(v, emu, w, cursor_y);
}

static void drawdebugger(const struct view *v, const struct emulator *emu)
{
    static const struct haltexpr empty = {.cond = HLT_NONE};

    int cursor_y = 0;
    werase(v->content);
    mvwprintw(v->content, cursor_y++, 0, "Tracing: %s",
              emu->args->tron ? "On" : "Off");
    mvwaddstr(v->content, cursor_y++, 0, "Reset Override: ");
    const int resetvector = debug_vector_override(emu->dbg);
    if (resetvector == NoResetVector) {
        waddstr(v->content, "None");
    } else {
        wprintw(v->content, "$%04X", resetvector);
    }
    const struct breakpoint *const bp =
        debug_bp_at(emu->dbg, emu->snapshot.debugger.halted);
    char break_desc[HEXPR_FMT_SIZE];
    const int err = haltexpr_desc(bp ? &bp->expr : &empty, break_desc);
    mvwprintw(v->content, cursor_y, 0, "Break: %s",
              err < 0 ? haltexpr_errstr(err) : break_desc);
}

static void drawcart(const struct view *v, const struct emulator *emu)
{
    static const char *const restrict namelabel = "Name: ";

    const int maxwidth = getmaxx(v->content) - (int)strlen(namelabel);
    int cursor_y = 0;
    mvwaddstr(v->content, cursor_y, 0, namelabel);
    const char
        *const cn = argparse_filename(emu->args->filepath),
        *endofname = strrchr(cn, '.');
    if (!endofname) {
        endofname = strrchr(cn, '\0');
    }
    const ptrdiff_t namelen = endofname - cn;
    const bool longname = namelen > maxwidth;
    // NOTE: ellipsis is one glyph wide despite being > 1 byte long
    wprintw(v->content, "%.*s%s", (int)(longname ? maxwidth - 1 : namelen),
            cn, longname ? "\u2026" : "");
    char fmtd[CART_FMT_SIZE];
    const int err = cart_format_extname(emu->cart, fmtd);
    mvwprintw(v->content, ++cursor_y, 0, "Format: %s",
              err < 0 ? cart_errstr(err) : fmtd);
}

static void drawinstructions(const struct view *v, uint16_t addr, int h, int y,
                             const struct emulator *emu)
{
    struct dis_instruction inst = {0};
    char disassembly[DIS_INST_SIZE];
    for (int i = 0; i < h - y; ++i) {
        int result = dis_parsemem_inst(emu->snapshot.mem.prglength,
                                       emu->snapshot.mem.currprg,
                                       inst.offset + inst.bv.size,
                                       &inst);
        if (result > 0) {
            result = dis_inst(addr, &inst, disassembly);
            if (result > 0) {
                mvwaddstr(v->content, i, 0, disassembly);
                addr += (uint16_t)inst.bv.size;
                continue;
            }
        }
        if (result < 0) {
            mvwaddstr(v->content, i, 0, dis_errstr(result));
        }
        break;
    }
}

static void drawvecs(const struct view *v, int h, int w, int y,
                     const struct emulator *emu)
{
    mvwhline(v->content, h - y--, 0, 0, w);

    uint8_t lo = emu->snapshot.mem.vectors[0],
            hi = emu->snapshot.mem.vectors[1];
    mvwprintw(v->content, h - y--, 0, "%04X: %02X %02X     NMI $%04X",
              CPU_VECTOR_NMI, lo, hi, bytowr(lo, hi));

    lo = emu->snapshot.mem.vectors[2];
    hi = emu->snapshot.mem.vectors[3];
    mvwprintw(v->content, h - y--, 0, "%04X: %02X %02X     RES",
              CPU_VECTOR_RES, lo, hi);
    const int resetvector = debug_vector_override(emu->dbg);
    if (resetvector == NoResetVector) {
        wprintw(v->content, " $%04X", bytowr(lo, hi));
    } else {
        wprintw(v->content, " " HEXPR_RES_IND "$%04X", resetvector);
    }

    lo = emu->snapshot.mem.vectors[4];
    hi = emu->snapshot.mem.vectors[5];
    mvwprintw(v->content, h - y, 0, "%04X: %02X %02X     IRQ $%04X",
              CPU_VECTOR_IRQ, lo, hi, bytowr(lo, hi));
}

static void drawprg(const struct view *v, const struct emulator *emu)
{
    static const int vector_offset = 4;

    int h, w;
    getmaxyx(v->content, h, w);
    werase(v->content);

    drawinstructions(v, emu->snapshot.datapath.current_instruction, h,
                     vector_offset, emu);
    drawvecs(v, h, w, vector_offset, emu);
}

static void drawregister(const struct view *v, const struct emulator *emu,
                         int *cursor_y)
{
    mvwprintw(v->content, (*cursor_y)++, 0, "PC: %04X  A: %02X",
              emu->snapshot.cpu.program_counter,
              emu->snapshot.cpu.accumulator);
    mvwprintw(v->content, (*cursor_y)++, 0, "S:  %02X    X: %02X",
              emu->snapshot.cpu.stack_pointer, emu->snapshot.cpu.xindex);
    mvwprintw(v->content, (*cursor_y)++, 0, "P:  %02X    Y: %02X",
              emu->snapshot.cpu.status, emu->snapshot.cpu.yindex);
}

static void drawflags(const struct view *v, const struct emulator *emu,
                      int *cursor_y)
{
    int cursor_x = 0;
    mvwaddstr(v->content, (*cursor_y)++, cursor_x, "N V - B D I Z C");
    for (size_t i = sizeof emu->snapshot.cpu.status * 8; i > 0; --i) {
        mvwprintw(v->content, *cursor_y, cursor_x, "%u",
                  byte_getbit(emu->snapshot.cpu.status, i - 1));
        cursor_x += 2;
    }
}

static void draw_cpu_line(const struct view *v, bool signal, int y, int x,
                          int dir_offset, const char *direction,
                          const char *label)
{
    if (!signal) {
        wattron(v->content, A_DIM);
    }
    mvwaddstr(v->content, y, x - 1, label);
    mvwaddstr(v->content, y + dir_offset, x, direction);
    if (!signal) {
        wattroff(v->content, A_DIM);
    }
}

static void draw_interrupt_latch(const struct view *v,
                                 enum csig_state interrupt, int y, int x)
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
        wattron(v->content, style);
    }
    mvwprintw(v->content, y, x, "%s%s", "\u25ef", modifier);
    if (style != A_NORMAL) {
        wattroff(v->content, style);
    }
}

static void drawdatapath(const struct view *v, const struct emulator *emu,
                         int cursor_y, int w)
{
    static const char
        *const restrict left = "\u21e6",
        *const restrict right = "\u21e8",
        *const restrict up = "\u2191",
        *const restrict down = "\u2193";
    static const int vsep1 = 7, vsep2 = 21, seph = 5;

    const int line_x = (w / 4) + 1;
    draw_cpu_line(v, emu->snapshot.lines.ready, cursor_y, line_x, 1, down,
                  "RDY");
    draw_cpu_line(v, emu->snapshot.lines.sync, cursor_y, line_x * 2, 1, up,
                  "SYNC");
    draw_cpu_line(v, emu->snapshot.lines.readwrite, cursor_y++, line_x * 3, 1,
                  up, "R/W\u0305");

    mvwhline(v->content, ++cursor_y, 0, 0, w);

    mvwvline(v->content, ++cursor_y, vsep1, 0, seph);
    mvwvline(v->content, cursor_y, vsep2, 0, seph);

    if (emu->snapshot.datapath.jammed) {
        wattron(v->content, A_STANDOUT);
        mvwaddstr(v->content, cursor_y, vsep1 + 2, " JAMMED ");
        wattroff(v->content, A_STANDOUT);
    } else {
        char buf[DIS_DATAP_SIZE];
        const int wlen = dis_datapath(&emu->snapshot, buf);
        const char *const mnemonic = wlen < 0 ? dis_errstr(wlen) : buf;
        mvwaddstr(v->content, cursor_y, vsep1 + 2, mnemonic);
    }

    mvwprintw(v->content, ++cursor_y, vsep1 + 2, "adl: %02X",
              emu->snapshot.datapath.addrlow_latch);

    mvwaddstr(v->content, ++cursor_y, 0, left);
    mvwprintw(v->content, cursor_y, 2, "%04X",
              emu->snapshot.datapath.addressbus);
    mvwprintw(v->content, cursor_y, vsep1 + 2, "adh: %02X",
              emu->snapshot.datapath.addrhigh_latch);
    const int dbus_x = vsep2 + 2;
    if (emu->snapshot.datapath.busfault) {
        mvwaddstr(v->content, cursor_y, dbus_x, "FLT");
    } else {
        mvwprintw(v->content, cursor_y, dbus_x, "%02X",
                  emu->snapshot.datapath.databus);
    }
    mvwaddstr(v->content, cursor_y, w - 1,
              emu->snapshot.lines.readwrite ? left : right);

    mvwprintw(v->content, ++cursor_y, vsep1 + 2, "adc: %02X",
              emu->snapshot.datapath.addrcarry_latch);

    mvwprintw(v->content, ++cursor_y, vsep1 + 2, "%*sT%u",
              emu->snapshot.datapath.exec_cycle, "",
              emu->snapshot.datapath.exec_cycle);

    mvwhline(v->content, ++cursor_y, 0, 0, w);

    draw_interrupt_latch(v, emu->snapshot.datapath.irq, cursor_y, line_x);
    draw_interrupt_latch(v, emu->snapshot.datapath.nmi, cursor_y, line_x * 2);
    draw_interrupt_latch(v, emu->snapshot.datapath.res, cursor_y, line_x * 3);
    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    draw_cpu_line(v, emu->snapshot.lines.irq, cursor_y, line_x, -1, up,
                  "I\u0305R\u0305Q\u0305");
    draw_cpu_line(v, emu->snapshot.lines.nmi, cursor_y, line_x * 2, -1, up,
                  "N\u0305M\u0305I\u0305");
    draw_cpu_line(v, emu->snapshot.lines.reset, cursor_y, line_x * 3, -1, up,
                  "R\u0305E\u0305S\u0305");
}

static void drawcpu(const struct view *v, const struct emulator *emu)
{
    const int w = getmaxx(v->content);
    int cursor_y = 0;
    werase(v->content);
    drawregister(v, emu, &cursor_y);
    mvwhline(v->content, cursor_y++, 0, 0, w);
    drawflags(v, emu, &cursor_y);
    mvwhline(v->content, ++cursor_y, 0, 0, w);
    drawdatapath(v, emu, ++cursor_y, w);
}

static void drawppu(const struct view *v, const struct emulator *emu)
{
    const int w = getmaxx(v->content);
    werase(v->content);
    mvwaddstr(v->content, 0, w / 2, "\u21d1");
    mvwprintw(v->content, 1, (w / 2) - 1, "%02X",
              emu->snapshot.ppu.register_databus);
    mvwprintw(v->content, 4, 0, "Pixel: (%d, %d)", emu->snapshot.ppu.line,
              emu->snapshot.ppu.dot);
}

static void drawram(const struct view *v, const struct emulator *emu)
{
    static const int start_x = 5;

    const int h = getmaxy(v->content);
    int cursor_x = start_x, cursor_y = 0;
    mvwvline(v->content, 0, start_x - 1, 0, h);
    const int page_count = (int)nes_ram_size(emu->console) / RamPageSize;
    for (int page = 0; page < page_count; ++page) {
        for (int page_row = 0; page_row < RamDim; ++page_row) {
            mvwprintw(v->content, cursor_y, 0, "%02X%X0", page, page_row);
            for (int page_col = 0; page_col < RamDim; ++page_col) {
                const size_t ramidx = (size_t)((page * RamPageSize)
                                               + (page_row * RamDim)
                                               + page_col);
                const bool sp = page == 1
                                && ramidx % (size_t)RamPageSize
                                    == emu->snapshot.cpu.stack_pointer;
                if (sp) {
                    wattron(v->content, A_STANDOUT);
                }
                mvwprintw(v->content, cursor_y, cursor_x, "%02X",
                          emu->snapshot.mem.ram[ramidx]);
                if (sp) {
                    wattroff(v->content, A_STANDOUT);
                }
                cursor_x += RamColWidth;
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
    v->content = derwin(v->win, h - 2, w - 2, 1, 1);
    v->inner = new_panel(v->content);
}

static void raminit(struct view *v, int h, int w, int y, int x, int ramsheets)
{
    static const int toprail_start = 7;

    createwin(v, h, w, y, x, "RAM");
    v->content = newpad((h - 4) * ramsheets, w - 2);
    v->inner = NULL;

    for (int col = 0; col < RamDim; ++col) {
        mvwprintw(v->win, 1, toprail_start + (col * RamColWidth), "%X", col);
    }
    mvwhline(v->win, 2, toprail_start - 1, 0, getmaxx(v->win) - toprail_start);
}

static void vcleanup(struct view *v)
{
    del_panel(v->inner);
    delwin(v->content);
    del_panel(v->outer);
    delwin(v->win);
    *v = (struct view){0};
}

static void ramrefresh(const struct view *v, const struct viewstate *vs)
{
    int ram_x, ram_y, ram_w, ram_h;
    getbegyx(v->win, ram_y, ram_x);
    getmaxyx(v->win, ram_h, ram_w);
    pnoutrefresh(v->content, (ram_h - 4) * vs->ramsheet, 0, ram_y + 3,
                 ram_x + 1, ram_y + ram_h - 2, ram_x + ram_w - 1);
}

//
// UI Loop Implementation
//

static void init_ui(struct layout *l, int ramsheets)
{
    static const int
        col1w = 30, col2w = 29, col3w = 29, col4w = 54, sysh = 23, crth = 4,
        cpuh = 20, maxh = 37, maxw = col1w + col2w + col3w + col4w;

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
        yoffset = (scrh - maxh) / 2,
        xoffset = (scrw - maxw) / 2;
    vinit(&l->system, sysh, col1w, yoffset, xoffset, "System");
    vinit(&l->debugger, maxh - sysh, col1w, yoffset + sysh, xoffset,
          "Debugger");
    vinit(&l->cart, crth, col2w, yoffset, xoffset + col1w, "Cart");
    vinit(&l->prg, maxh - crth, col2w, yoffset + crth, xoffset + col1w, "PRG");
    vinit(&l->cpu, cpuh, col3w, yoffset, xoffset + col1w + col2w, "CPU");
    vinit(&l->ppu, maxh - cpuh, col3w, yoffset + cpuh, xoffset + col1w + col2w,
          "PPU");
    raminit(&l->ram, maxh, col4w, yoffset, xoffset + col1w + col2w + col3w,
            ramsheets);
}

static void tick_start(struct viewstate *vs, const struct emulator *emu)
{
    cycleclock_tickstart(&vs->clock.cyclock, !emu->snapshot.lines.ready);
}

static void tick_end(struct runclock *c)
{
    cycleclock_tickend(&c->cyclock);
    tick_sleep(c);
}

static void handle_input(struct viewstate *vs, const struct emulator *emu)
{
    const int input = getch();
    switch (input) {
    case ' ':
        nes_ready(emu->console, !emu->snapshot.lines.ready);
        break;
    case '=':   // "Lowercase" +
        ++vs->clock.cyclock.cycles_per_sec;
        goto pclamp_cps;
    case '+':
        vs->clock.cyclock.cycles_per_sec += 10;
    pclamp_cps:
        if (vs->clock.cyclock.cycles_per_sec > MaxCps) {
            vs->clock.cyclock.cycles_per_sec = MaxCps;
        }
        break;
    case '-':
        --vs->clock.cyclock.cycles_per_sec;
        goto nclamp_cps;
    case '_':   // "Uppercase" -
        vs->clock.cyclock.cycles_per_sec -= 10;
    nclamp_cps:
        if (vs->clock.cyclock.cycles_per_sec < MinCps) {
            vs->clock.cyclock.cycles_per_sec = MinCps;
        }
        break;
    case 'i':
        nes_interrupt(emu->console, CSGI_IRQ, emu->snapshot.lines.irq);
        break;
    case 'm':
        nes_set_mode(emu->console, nes_mode(emu->console) + 1);
        break;
    case 'M':
        nes_set_mode(emu->console, nes_mode(emu->console) - 1);
        break;
    case 'n':
        nes_interrupt(emu->console, CSGI_NMI, emu->snapshot.lines.nmi);
        break;
    case 'q':
        vs->running = false;
        break;
    case 'r':
        vs->ramsheet = (vs->ramsheet + 1) % vs->total_ramsheets;
        break;
    case 'R':
        --vs->ramsheet;
        if (vs->ramsheet < 0) {
            vs->ramsheet = vs->total_ramsheets - 1;
        }
        break;
    case 's':
        nes_interrupt(emu->console, CSGI_RES, emu->snapshot.lines.reset);
        break;
    }
}

static void emu_update(struct emulator *emu, struct viewstate *vs)
{
    nes_cycle(emu->console, &vs->clock.cyclock);
    nes_snapshot(emu->console, &emu->snapshot);
}

static void refresh_ui(const struct layout *l, const struct viewstate *vs,
                       const struct emulator *emu)
{
    drawsystem(&l->system, vs, emu);
    drawdebugger(&l->debugger, emu);
    drawcart(&l->cart, emu);
    drawprg(&l->prg, emu);
    drawcpu(&l->cpu, emu);
    drawppu(&l->ppu, emu);
    drawram(&l->ram, emu);

    update_panels();
    ramrefresh(&l->ram, vs);
    doupdate();
}

static void cleanup_ui(struct layout *l)
{
    vcleanup(&l->ram);
    vcleanup(&l->ppu);
    vcleanup(&l->cpu);
    vcleanup(&l->prg);
    vcleanup(&l->cart);
    vcleanup(&l->debugger);
    vcleanup(&l->system);

    endwin();
}

//
// Public Interface
//

int ui_curses_loop(struct emulator *emu)
{
    assert(emu != NULL);

    struct viewstate state = {
        .clock = {.cyclock = {.cycles_per_sec = 4}},
        .running = true,
        .total_ramsheets = (int)nes_ram_size(emu->console) / RamSheetSize,
    };
    struct layout layout;
    init_ui(&layout, state.total_ramsheets);
    cycleclock_start(&state.clock.cyclock);
    do {
        tick_start(&state, emu);
        handle_input(&state, emu);
        if (state.running) {
            emu_update(emu, &state);
            refresh_ui(&layout, &state, emu);
        }
        tick_end(&state.clock);
    } while (state.running);
    cleanup_ui(&layout);

    return 0;
}

const char *ui_curses_version(void)
{
    return curses_version();
}

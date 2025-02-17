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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

//
// MARK: - Run Loop Clock
//

static const int
    DisplayHz = 60, RamColWidth = 3, RamDim = 16,
    RamPageSize = RamDim * RamDim;

enum ram_selection {
    RSEL_RAM,
    RSEL_VRAM,
    RSEL_PPU,
    RSEL_COUNT,
};

struct viewstate {
    struct runclock {
        struct aldo_clock clock;
        double tickleft_ms;
        int oldrate;
        enum aldo_clockscale scale;
    } clock;
    enum ram_selection ramselect;
    int ramsheet, total_ramsheets;
    bool running;
};

static void tick_sleep(struct runclock *c)
{
    static const struct timespec vsync = {
        .tv_nsec = ALDO_NS_PER_S / DisplayHz,
    };

    struct timespec elapsed = aldo_elapsed(&c->clock.current);

    // NOTE: if elapsed nanoseconds is greater than vsync we're over
    // our time budget; if elapsed *seconds* is greater than vsync
    // we've BLOWN AWAY our time budget; either way don't sleep.
    if (elapsed.tv_nsec > vsync.tv_nsec || elapsed.tv_sec > vsync.tv_sec) {
        // NOTE: we've already blown the tick time, so convert everything
        // to milliseconds to make the math easier.
        c->tickleft_ms = aldo_timespec_to_ms(&vsync)
                            - aldo_timespec_to_ms(&elapsed);
        return;
    }

    struct timespec tick_left = {
        .tv_nsec = vsync.tv_nsec - elapsed.tv_nsec,
    };
    c->tickleft_ms = aldo_timespec_to_ms(&tick_left);

    aldo_sleep(tick_left);
}

//
// MARK: - UI Widgets
//

static const char
    *const restrict ArrowUp = "\u2191",
    *const restrict ArrowRight = "\u2192",
    *const restrict ArrowDown = "\u2193",
    *const restrict DArrowLeft = "\u21e6",
    *const restrict DArrowUp = "\u21e7",
    *const restrict DArrowRight = "\u21e8",
    *const restrict DArrowDown = "\u21e9";

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

static int drawstats(const struct view *v, int cursor_y,
                     const struct viewstate *vs, const struct cliargs *args)
{
    // NOTE: update timing metrics on a readable interval
    static const double refresh_interval_ms = 250;
    static double display_tickleft, display_ticktime, refreshdt;

    const struct aldo_clock *clock = &vs->clock.clock;
    if ((refreshdt += clock->ticktime_ms) >= refresh_interval_ms) {
        display_tickleft = vs->clock.tickleft_ms;
        display_ticktime = clock->ticktime_ms;
        refreshdt = 0;
    }

    mvwprintw(v->content, cursor_y++, 0, "Display Hz: %d (%.2f)", DisplayHz,
              (double)clock->ticks / clock->runtime);
    if (display_tickleft < 0) {
        wattron(v->content, A_STANDOUT);
    }
    mvwprintw(v->content, cursor_y++, 0, "\u0394T: %.3f (%+.3f)",
              display_ticktime, display_tickleft);
    if (display_tickleft < 0) {
        wattroff(v->content, A_STANDOUT);
    }
    mvwprintw(v->content, cursor_y++, 0, "Ticks: %" PRIu64, clock->ticks);
    mvwprintw(v->content, cursor_y++, 0, "Runtime: %.3f", clock->runtime);
    mvwprintw(v->content, cursor_y++, 0, "Emutime: %.3f", clock->emutime);
    mvwprintw(v->content, cursor_y++, 0, "Frames: %" PRIu64, clock->frames);
    mvwprintw(v->content, cursor_y++, 0, "Cycles: %" PRIu64, clock->cycles);
    mvwprintw(v->content, cursor_y++, 0, "%s: %d",
              vs->clock.scale == ALDO_CS_CYCLE
                ? "Cycles per Second"
                : "Frames per Second", clock->rate);
    mvwprintw(v->content, cursor_y++, 0, "BCD Supported: %s",
              args->bcdsupport ? "Yes" : "No");
    return cursor_y;
}

static void drawcontrols(const struct view *v, const struct emulator *emu,
                         int w, int cursor_y)
{
    static const char *const restrict halt = "HALT";

    wmove(v->content, cursor_y, 0);

    double center_offset = (w - (int)strlen(halt)) / 2.0;
    assert(center_offset > 0);
    char halt_label[w + 1];
    snprintf(halt_label, sizeof halt_label, "%*s%s%*s",
             (int)round(center_offset), "", halt,
             (int)floor(center_offset), "");
    drawtoggle(v, halt_label, !emu->snapshot.cpu.lines.ready);

    cursor_y += 2;
    enum aldo_execmode mode = aldo_nes_mode(emu->console);
    mvwaddstr(v->content, cursor_y, 0, "Mode:");
    drawtoggle(v, " Sub ", mode == ALDO_EXC_SUBCYCLE);
    drawtoggle(v, " Cycle ", mode == ALDO_EXC_CYCLE);
    drawtoggle(v, " Step ", mode == ALDO_EXC_STEP);
    drawtoggle(v, " Run ", mode == ALDO_EXC_RUN);

    cursor_y += 2;
    mvwaddstr(v->content, cursor_y, 0, "Signal:");
    drawtoggle(v, " IRQ ", aldo_nes_probe(emu->console, ALDO_INT_IRQ));
    drawtoggle(v, " NMI ", aldo_nes_probe(emu->console, ALDO_INT_NMI));
    drawtoggle(v, " RST ", aldo_nes_probe(emu->console, ALDO_INT_RST));

    mvwhline(v->content, ++cursor_y, 0, 0, w);
    mvwaddstr(v->content, ++cursor_y, 0, "Halt/Run: <Space>");
    mvwaddstr(v->content, ++cursor_y, 0, "Run Mode: m/M");
    mvwaddstr(v->content, ++cursor_y, 0, "Signal: i, n, s");
    mvwaddstr(v->content, ++cursor_y, 0,
              "Speed \u00b11 (\u00b110): -/= (_/+)");
    mvwaddstr(v->content, ++cursor_y, 0, "Clock Scale: c");
    mvwaddstr(v->content, ++cursor_y, 0, "Ram: r/R  Fwd/Bck: f/b");
    mvwaddstr(v->content, ++cursor_y, 0, "Quit: q");
}

static void drawsystem(const struct view *v, const struct viewstate *vs,
                       const struct emulator *emu)
{
    int w = getmaxx(v->content);
    werase(v->content);
    int cursor_y = drawstats(v, 0, vs, emu->args);
    mvwhline(v->content, cursor_y++, 0, 0, w);
    drawcontrols(v, emu, w, cursor_y);
}

static void drawdebugger(const struct view *v, const struct emulator *emu)
{
    static const struct aldo_haltexpr empty = {.cond = ALDO_HLT_NONE};

    int cursor_y = 0;
    werase(v->content);
    mvwprintw(v->content, cursor_y++, 0, "Tracing: %s",
              emu->args->tron ? "On" : "Off");
    mvwaddstr(v->content, cursor_y++, 0, "Reset Override: ");
    int resetvector = aldo_debug_vector_override(emu->debugger);
    if (resetvector == Aldo_NoResetVector) {
        waddstr(v->content, "None");
    } else {
        wprintw(v->content, "$%04X", resetvector);
    }
    const struct aldo_breakpoint *bp = aldo_debug_halted(emu->debugger);
    char break_desc[ALDO_HEXPR_FMT_SIZE];
    int err = aldo_haltexpr_desc(bp ? &bp->expr : &empty, break_desc);
    mvwprintw(v->content, cursor_y, 0, "Break: %s",
              err < 0 ? aldo_haltexpr_errstr(err) : break_desc);
}

static void drawcart(const struct view *v, const struct emulator *emu)
{
    static const char *const restrict namelabel = "Name: ";

    int maxwidth = getmaxx(v->content) - (int)strlen(namelabel), cursor_y = 0;
    mvwaddstr(v->content, cursor_y, 0, namelabel);
    const char
        *cn = argparse_filename(emu->args->filepath),
        *endofname = strrchr(cn, '.');
    if (!endofname) {
        endofname = strrchr(cn, '\0');
    }
    ptrdiff_t namelen = endofname - cn;
    bool longname = namelen > maxwidth;
    // NOTE: ellipsis is one glyph wide despite being > 1 byte long
    wprintw(v->content, "%.*s%s", (int)(longname ? maxwidth - 1 : namelen),
            cn, longname ? "\u2026" : "");
    char fmtd[ALDO_CART_FMT_SIZE];
    int err = aldo_cart_format_extname(emu->cart, fmtd);
    mvwprintw(v->content, ++cursor_y, 0, "Format: %s",
              err < 0 ? aldo_cart_errstr(err) : fmtd);
}

static void drawinstructions(const struct view *v, int h, int y,
                             const struct aldo_snapshot *snp)
{
    struct aldo_dis_instruction inst = {0};
    uint16_t addr = snp->cpu.datapath.current_instruction;
    char disassembly[ALDO_DIS_INST_SIZE];
    for (int i = 0; i < h - y; ++i) {
        int result = aldo_dis_parsemem_inst(snp->prg.curr->length,
                                            snp->prg.curr->pc,
                                            inst.offset + inst.bv.size, &inst);
        if (result > 0) {
            result = aldo_dis_inst(addr, &inst, disassembly);
            if (result > 0) {
                mvwaddstr(v->content, i, 0, disassembly);
                addr += (uint16_t)inst.bv.size;
                continue;
            }
        }
        if (result < 0) {
            mvwaddstr(v->content, i, 0, aldo_dis_errstr(result));
        }
        break;
    }
}

static void drawvecs(const struct view *v, int h, int w, int y,
                     const struct emulator *emu)
{
    mvwhline(v->content, h - y--, 0, 0, w);

    const uint8_t *vectors = emu->snapshot.prg.vectors;
    uint8_t lo = vectors[0],
            hi = vectors[1];
    mvwprintw(v->content, h - y--, 0, "%4X: %02X %02X     NMI $%04X",
              ALDO_CPU_VECTOR_NMI, lo, hi, aldo_bytowr(lo, hi));

    lo = vectors[2];
    hi = vectors[3];
    mvwprintw(v->content, h - y--, 0, "%4X: %02X %02X     RST",
              ALDO_CPU_VECTOR_RST, lo, hi);
    int resetvector = aldo_debug_vector_override(emu->debugger);
    if (resetvector == Aldo_NoResetVector) {
        wprintw(v->content, " $%04X", aldo_bytowr(lo, hi));
    } else {
        wprintw(v->content, " " ALDO_HEXPR_RST_IND "$%04X", resetvector);
    }

    lo = vectors[4];
    hi = vectors[5];
    mvwprintw(v->content, h - y, 0, "%4X: %02X %02X     IRQ $%04X",
              ALDO_CPU_VECTOR_IRQ, lo, hi, aldo_bytowr(lo, hi));
}

static void drawprg(const struct view *v, const struct emulator *emu)
{
    static const int vector_offset = 4;

    int h, w;
    getmaxyx(v->content, h, w);
    werase(v->content);

    drawinstructions(v, h, vector_offset, &emu->snapshot);
    drawvecs(v, h, w, vector_offset, emu);
}

static int drawregisters(const struct view *v, int cursor_y,
                         const struct aldo_snapshot *snp)
{
    mvwprintw(v->content, cursor_y++, 0, "PC: %04X  A: %02X",
              snp->cpu.program_counter, snp->cpu.accumulator);
    mvwprintw(v->content, cursor_y++, 0, "S:  %02X    X: %02X",
              snp->cpu.stack_pointer, snp->cpu.xindex);
    mvwprintw(v->content, cursor_y++, 0, "P:  %02X    Y: %02X",
              snp->cpu.status, snp->cpu.yindex);
    return cursor_y;
}

static int drawflags(const struct view *v, int cursor_y,
                     const struct aldo_snapshot *snp)
{
    int cursor_x = 0;
    mvwaddstr(v->content, cursor_y++, cursor_x, "N V - B D I Z C");
    for (size_t i = sizeof snp->cpu.status * 8; i > 0; --i) {
        mvwprintw(v->content, cursor_y, cursor_x, "%d",
                  aldo_getbit(snp->cpu.status, i - 1));
        cursor_x += 2;
    }
    return cursor_y;
}

static void draw_chip_vline(const struct view *v, bool signal, int y, int x,
                            int dir_offset, const char *direction,
                            int lbl_offset, const char *label)
{
    if (!signal) {
        wattron(v->content, A_DIM);
    }
    mvwaddstr(v->content, y, x + lbl_offset, label);
    mvwaddstr(v->content, y + dir_offset, x, direction);
    if (!signal) {
        wattroff(v->content, A_DIM);
    }
}

static void draw_chip_hline(const struct view *v, bool signal, int y, int x,
                            const char *label, const char *direction)
{
    if (!signal) {
        wattron(v->content, A_DIM);
    }
    mvwprintw(v->content, y, x, "%s %s", label, direction);
    if (!signal) {
        wattroff(v->content, A_DIM);
    }
}

static void draw_interrupt_latch(const struct view *v,
                                 enum aldo_sigstate interrupt, int y, int x)
{
    const char *modifier = "";
    attr_t style = A_NORMAL;
    switch (interrupt) {
    case ALDO_SIG_CLEAR:
        // NOTE: draw nothing for CLEAR state
        return;
    case ALDO_SIG_DETECTED:
        style = A_DIM;
        break;
    case ALDO_SIG_COMMITTED:
        style = A_UNDERLINE;
        modifier = "\u0305";
        break;
    case ALDO_SIG_SERVICED:
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

static void drawdatapath(const struct view *v, int cursor_y, int w,
                         const struct aldo_snapshot *snp)
{
    static const int
        seph = 5, vsep1 = 7, vsep2 = 21, col1 = vsep1 + 2, col2 = vsep2 + 2;

    int line_x = (w / 4) + 1;
    draw_chip_vline(v, snp->cpu.lines.ready, cursor_y, line_x, 1, ArrowDown,
                    -1, "RDY");
    draw_chip_vline(v, snp->cpu.lines.sync, cursor_y, line_x * 2, 1, ArrowUp,
                    -1, "SYNC");
    draw_chip_vline(v, snp->cpu.lines.readwrite, cursor_y++, line_x * 3, 1,
                    ArrowUp, -1, "R/W\u0305");

    mvwhline(v->content, ++cursor_y, 0, 0, w);

    mvwvline(v->content, ++cursor_y, vsep1, 0, seph);
    mvwvline(v->content, cursor_y, vsep2, 0, seph);

    if (snp->cpu.datapath.jammed) {
        wattron(v->content, A_STANDOUT);
        mvwaddstr(v->content, cursor_y, col1, " JAMMED ");
        wattroff(v->content, A_STANDOUT);
    } else {
        char buf[ALDO_DIS_DATAP_SIZE];
        int wlen = aldo_dis_datapath(snp, buf);
        const char *mnemonic = wlen < 0 ? aldo_dis_errstr(wlen) : buf;
        mvwaddstr(v->content, cursor_y, col1, mnemonic);
    }

    mvwprintw(v->content, ++cursor_y, col1, "adl: %02X",
              snp->cpu.datapath.addrlow_latch);

    mvwaddstr(v->content, ++cursor_y, 0, DArrowLeft);
    mvwprintw(v->content, cursor_y, 2, "%04X", snp->cpu.datapath.addressbus);
    mvwprintw(v->content, cursor_y, col1, "adh: %02X",
              snp->cpu.datapath.addrhigh_latch);
    if (snp->cpu.datapath.busfault) {
        mvwaddstr(v->content, cursor_y, col2, "FLT");
    } else {
        mvwprintw(v->content, cursor_y, col2, "%02X",
                  snp->cpu.datapath.databus);
    }
    mvwaddstr(v->content, cursor_y, w - 1,
              snp->cpu.lines.readwrite ? DArrowLeft : DArrowRight);

    mvwprintw(v->content, ++cursor_y, col1, "adc: %02X",
              snp->cpu.datapath.addrcarry_latch);

    mvwprintw(v->content, ++cursor_y, col1, "%*sT%d",
              snp->cpu.datapath.exec_cycle, "", snp->cpu.datapath.exec_cycle);

    mvwhline(v->content, ++cursor_y, 0, 0, w);

    draw_interrupt_latch(v, snp->cpu.datapath.irq, cursor_y, line_x);
    draw_interrupt_latch(v, snp->cpu.datapath.nmi, cursor_y, line_x * 2);
    draw_interrupt_latch(v, snp->cpu.datapath.rst, cursor_y, line_x * 3);
    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    draw_chip_vline(v, snp->cpu.lines.irq, cursor_y, line_x, -1, ArrowUp, -1,
                    "I\u0305R\u0305Q\u0305");
    draw_chip_vline(v, snp->cpu.lines.nmi, cursor_y, line_x * 2, -1, ArrowUp,
                    -1, "N\u0305M\u0305I\u0305");
    draw_chip_vline(v, snp->cpu.lines.reset, cursor_y, line_x * 3, -1, ArrowUp,
                    -1, "R\u0305S\u0305T\u0305");
}

static void drawcpu(const struct view *v, const struct aldo_snapshot *snp)
{
    int w = getmaxx(v->content);
    werase(v->content);
    int cursor_y = drawregisters(v, 0, snp);
    mvwhline(v->content, cursor_y++, 0, 0, w);
    cursor_y = drawflags(v, cursor_y, snp);
    mvwhline(v->content, ++cursor_y, 0, 0, w);
    drawdatapath(v, ++cursor_y, w, snp);
}

static int drawtop_plines(const struct view *v, int cursor_y, int line_x,
                          int w, const struct aldo_snapshot *snp)
{
    uint8_t sel = snp->ppu.pipeline.register_select;
    char sel_buf[4];
    sprintf(sel_buf, "%d%d%d", aldo_getbit(sel, 2), aldo_getbit(sel, 1),
            aldo_getbit(sel, 0));
    draw_chip_vline(v, snp->ppu.lines.cpu_readwrite, cursor_y, line_x, 1,
                    ArrowDown, -1, sel_buf);
    draw_chip_vline(v, snp->ppu.lines.interrupt, cursor_y, line_x * 2, 1,
                    ArrowUp, -1, "I\u0305N\u0305T\u0305");
    draw_chip_vline(v, snp->ppu.lines.reset, cursor_y++, line_x * 3, 1,
                    ArrowDown, -1, "R\u0305S\u0305T\u0305");

    mvwhline(v->content, ++cursor_y, 0, 0, w);
    draw_interrupt_latch(v, snp->ppu.pipeline.rst, cursor_y, line_x * 3);

    mvwprintw(v->content, cursor_y, line_x - 2, "[%s%02X]",
              snp->ppu.lines.cpu_readwrite ? DArrowUp : DArrowDown,
              snp->ppu.pipeline.register_databus);
    return cursor_y;
}

static int draw_pregisters(const struct view *v, int cursor_y,
                           const struct aldo_snapshot *snp)
{
    uint8_t status = snp->ppu.status;
    mvwprintw(v->content, ++cursor_y, 0, "CONTROL: %02X  STATUS:  %d%d%d",
              snp->ppu.ctrl, aldo_getbit(status, 7), aldo_getbit(status, 6),
              aldo_getbit(status, 5));
    mvwprintw(v->content, ++cursor_y, 0, "MASK:    %02X  OAMADDR: %02X",
              snp->ppu.mask, snp->ppu.oamaddr);
    return cursor_y + 1;
}

static void draw_scroll_addr(const struct view *v, int y, int x, char label,
                             uint16_t val)
{
    mvwprintw(v->content, y, x, "%c: %04X (%d,%d,%02d,%02d)", label, val,
              (val & 0x7000) >> 12, (val & 0xc00) >> 10, (val & 0x3e0) >> 5,
              val & 0x1f);
}

static int draw_pipeline(const struct view *v, int cursor_y, int w,
                         const struct aldo_snapshot *snp)
{
    static const int seph = 5, vsep = 19, buscol = vsep + 2;

    mvwvline(v->content, ++cursor_y, vsep, 0, seph);

    draw_scroll_addr(v, cursor_y, 0, 'v', snp->ppu.pipeline.scrolladdr);
    mvwprintw(v->content, cursor_y, buscol, "%04X",
              snp->ppu.pipeline.addressbus);
    mvwaddstr(v->content, cursor_y, w - 1, DArrowRight);

    draw_scroll_addr(v, ++cursor_y, 0, 't', snp->ppu.pipeline.tempaddr);
    draw_chip_hline(v, snp->ppu.lines.address_enable, cursor_y, buscol + 1,
                    "ALE", ArrowRight);

    mvwprintw(v->content, ++cursor_y, 0, "x: %02X (%d)",
              snp->ppu.pipeline.xfine, snp->ppu.pipeline.xfine);
    draw_chip_hline(v, snp->ppu.lines.read, cursor_y, buscol + 3, "R\u0305",
                    ArrowRight);

    mvwprintw(v->content, ++cursor_y, 0, "c: %d o: %d w: %d",
              snp->ppu.pipeline.cv_pending, snp->ppu.pipeline.oddframe,
              snp->ppu.pipeline.writelatch);
    draw_chip_hline(v, snp->ppu.lines.write, cursor_y, buscol + 3, "W\u0305",
                    ArrowRight);

    mvwprintw(v->content, ++cursor_y, 0, "r: %02X",
              snp->ppu.pipeline.readbuffer);
    if (snp->ppu.pipeline.busfault) {
        mvwaddstr(v->content, cursor_y, buscol + 1, "FLT");
    } else {
        mvwprintw(v->content, cursor_y, buscol + 1, " %02X",
                  snp->ppu.pipeline.databus);
    }
    // NOTE: write line does not signal when writing to palette ram so use the
    // read signal to determine direction of databus flow.
    mvwaddstr(v->content, cursor_y, w - 1,
              snp->ppu.lines.read ? DArrowRight : DArrowLeft);
    return cursor_y;
}

static void drawbottom_plines(const struct view *v, int cursor_y, int line_x,
                              const struct aldo_snapshot *snp)
{
    mvwprintw(v->content, cursor_y, line_x - 2, "[%s%02X]", DArrowDown,
              snp->ppu.pipeline.pixel);

    char vbuf[10];
    sprintf(vbuf, "(%3d,%3d)", snp->ppu.pipeline.line, snp->ppu.pipeline.dot);
    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    draw_chip_vline(v, snp->ppu.lines.video_out, cursor_y, line_x, -1,
                    ArrowDown, -4, vbuf);
}

static void drawppu(const struct view *v, const struct aldo_snapshot *snp)
{
    int w = getmaxx(v->content), line_x = (w / 4) + 1;
    int cursor_y = drawtop_plines(v, 0, line_x, w, snp);
    cursor_y = draw_pregisters(v, cursor_y, snp);
    mvwhline(v->content, ++cursor_y, 0, 0, w);
    cursor_y = draw_pipeline(v, cursor_y, w, snp);
    mvwhline(v->content, ++cursor_y, 0, 0, w);
    drawbottom_plines(v, cursor_y, line_x, snp);
}

static void drawramtitle(const struct view *v, const struct viewstate *vs)
{
    static const int titlew = 16, offsets[] = {2, 7, 13};
    static const char *const restrict labels[] = {"RAM", "VRAM", "PPU"};

    mvwhline(v->win, 0, 1, 0, titlew);
    for (size_t i = 0; i < RSEL_COUNT; ++i) {
        if (i == vs->ramselect) {
            mvwprintw(v->win, 0, offsets[i] - 1, "[%s]", labels[i]);
        } else {
            mvwaddstr(v->win, 0, offsets[i], labels[i]);
        }
    }
}

static int draw_mempage(const struct view *v, const struct emulator *emu,
                        const uint8_t *mem, enum ram_selection sel,
                        int start_x, int cursor_y, int page, int page_offset,
                        int page_rows)
{
    int cursor_x = start_x;
    size_t skipped = 0;
    for (int page_row = 0; page_row < page_rows; ++page_row) {
        if (sel != RSEL_PPU && page == 1) {
            // NOTE: clear any leftover PPU diagrams
            wclrtoeol(v->content);
        }
        mvwprintw(v->content, cursor_y, 0, "%02X%X0", page + page_offset,
                  page_row);
        for (int page_col = 0; page_col < RamDim; ++page_col) {
            size_t ramidx = (size_t)((page * RamPageSize) + (page_row * RamDim)
                                     + page_col);
            // NOTE: skip over palette's mirrored addresses
            if (sel == RSEL_PPU && page_offset == 0x3f
                && (ramidx & 0x13) == 0x10) {
                mvwaddstr(v->content, cursor_y, cursor_x, "--");
                ++skipped;
            } else {
                bool sp = sel == RSEL_RAM && page == 1
                            && ramidx % (size_t)RamPageSize
                                == emu->snapshot.cpu.stack_pointer;
                if (sp) {
                    wattron(v->content, A_STANDOUT);
                }
                mvwprintw(v->content, cursor_y, cursor_x, "%02X",
                          mem[ramidx - skipped]);
                if (sp) {
                    wattroff(v->content, A_STANDOUT);
                }
            }
            cursor_x += RamColWidth;
        }
        cursor_x = start_x;
        ++cursor_y;
    }
    if (page % 2 == 0) {
        ++cursor_y;
    }
    return cursor_y;
}

static void draw_membanks(const struct view *v, const struct viewstate *vs,
                          const struct emulator *emu, int start_x)
{
    int cursor_y = 0, page_offset;
    const uint8_t *mem;
    if (vs->ramselect == RSEL_VRAM) {
        page_offset = 0x20;
        mem = emu->snapshot.mem.vram;
    } else {
        page_offset = 0;
        mem = emu->snapshot.mem.ram;
    }
    int page_count = vs->total_ramsheets * 2;
    for (int page = 0; page < page_count; ++page) {
        cursor_y = draw_mempage(v, emu, mem, vs->ramselect, start_x, cursor_y,
                                page, page_offset, RamDim);
    }
    mvwvline(v->content, 0, start_x - 1, 0, getmaxy(v->content));
}

static void draw_ppumem(const struct view *v, const struct emulator *emu,
                        int start_x)
{
    static const int sheeth = (RamDim * 2) + 1;

    const struct aldo_snapshot *snp = &emu->snapshot;
    int cursor_y = draw_mempage(v, emu, snp->mem.oam, RSEL_PPU, start_x, 0, 0,
                                0, RamDim);
    wclrtoeol(v->content);
    cursor_y = draw_mempage(v, emu, snp->mem.secondary_oam, RSEL_PPU, start_x,
                            cursor_y, 0, 0, 2);
    wclrtoeol(v->content);
    cursor_y = draw_mempage(v, emu, snp->mem.palette, RSEL_PPU, start_x,
                            cursor_y, 0, 0x3f, 2);
    mvwhline(v->content, cursor_y - 1, 0, 0, getmaxx(v->content));
    do {
        wmove(v->content, cursor_y, 0);
        wclrtoeol(v->content);
    } while (++cursor_y < sheeth);
}

static void drawram(const struct view *v, const struct viewstate *vs,
                    const struct emulator *emu)
{
    static const int start_x = 5;

    drawramtitle(v, vs);

    if (vs->ramselect == RSEL_PPU) {
        draw_ppumem(v, emu, start_x);
    } else {
        draw_membanks(v, vs, emu, start_x);
    }
}

static void createwin(struct view *v, int h, int w, int y, int x,
                      const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->outer = new_panel(v->win);
    box(v->win, 0, 0);
    if (title) {
        mvwaddstr(v->win, 0, 1, title);
    }
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

    createwin(v, h, w, y, x, NULL);
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
    int sheet = vs->ramselect == RSEL_PPU ? 0 : vs->ramsheet;
    pnoutrefresh(v->content, (ram_h - 4) * sheet, 0, ram_y + 3, ram_x + 1,
                 ram_y + ram_h - 2, ram_x + ram_w - 1);
}

//
// MARK: - UI Loop Implementation
//

static void init_ui(struct layout *l, int ramsheets)
{
    static const int
        col1w = 30, col2w = 29, col3w = 29, col4w = 54, sysh = 25, crth = 4,
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
    int yoffset = (scrh - maxh) / 2, xoffset = (scrw - maxw) / 2;
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
    aldo_clock_tickstart(&vs->clock.clock, !emu->snapshot.cpu.lines.ready);
}

static void tick_end(struct runclock *c)
{
    aldo_clock_tickend(&c->clock);
    tick_sleep(c);
}

static void applyrate(int *spd, int adjustment, int min, int max)
{
    *spd += adjustment;
    if (*spd < min) {
        *spd = min;
    } else if (*spd > max) {
        *spd = max;
    }
}

static void adjustrate(struct viewstate *vs, int adjustment)
{
    int min, max;
    if (vs->clock.scale == ALDO_CS_CYCLE) {
        min = Aldo_MinCps;
        max = Aldo_MaxCps;
    } else {
        min = Aldo_MinFps;
        max = Aldo_MaxFps;
    }
    applyrate(&vs->clock.clock.rate, adjustment, min, max);
}

static void selectrate(struct runclock *clock)
{
    int prev = clock->oldrate;
    clock->oldrate = clock->clock.rate;
    clock->clock.rate = prev;
    clock->scale = !clock->scale;
    clock->clock.rate_factor = clock->scale == ALDO_CS_CYCLE
                                ? aldo_nes_cycle_factor()
                                : aldo_nes_frame_factor();
}

static void handle_input(struct viewstate *vs, const struct emulator *emu)
{
    int input = getch();
    switch (input) {
    case ' ':
        aldo_nes_ready(emu->console, !emu->snapshot.cpu.lines.ready);
        break;
    case '=':   // "Lowercase" +
        adjustrate(vs, 1);
        break;
    case '+':
        adjustrate(vs, 10);
        break;
    case '-':
        adjustrate(vs, -1);
        break;
    case '_':   // "Uppercase" -
        adjustrate(vs, -10);
        break;
    case 'b':
        if (vs->ramselect != RSEL_PPU) {
            if (--vs->ramsheet < 0) {
                vs->ramsheet = vs->total_ramsheets - 1;
            }
        }
        break;
    case 'c':
        selectrate(&vs->clock);
        break;
    case 'f':
        if (vs->ramselect != RSEL_PPU) {
            vs->ramsheet = (vs->ramsheet + 1) % vs->total_ramsheets;
        }
        break;
    case 'i':
        aldo_nes_set_probe(emu->console, ALDO_INT_IRQ,
                           !aldo_nes_probe(emu->console, ALDO_INT_IRQ));
        break;
    case 'm':
        aldo_nes_set_mode(emu->console, aldo_nes_mode(emu->console) + 1);
        break;
    case 'M':
        aldo_nes_set_mode(emu->console, aldo_nes_mode(emu->console) - 1);
        break;
    case 'n':
        aldo_nes_set_probe(emu->console, ALDO_INT_NMI,
                           !aldo_nes_probe(emu->console, ALDO_INT_NMI));
        break;
    case 'q':
        vs->running = false;
        break;
    case 'r':
        vs->ramselect = (vs->ramselect + 1) % RSEL_COUNT;
        break;
    case 'R':
        if ((int)--vs->ramselect < 0) {
            vs->ramselect = RSEL_COUNT - 1;
        }
        break;
    case 's':
        aldo_nes_set_probe(emu->console, ALDO_INT_RST,
                           !aldo_nes_probe(emu->console, ALDO_INT_RST));
        break;
    }
}

static void refresh_ui(const struct layout *l, const struct viewstate *vs,
                       const struct emulator *emu)
{
    drawsystem(&l->system, vs, emu);
    drawdebugger(&l->debugger, emu);
    drawcart(&l->cart, emu);
    drawprg(&l->prg, emu);
    drawcpu(&l->cpu, &emu->snapshot);
    drawppu(&l->ppu, &emu->snapshot);
    drawram(&l->ram, vs, emu);

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
// MARK: - Public Interface
//

int ui_curses_loop(struct emulator *emu)
{
    assert(emu != NULL);

    static const int sheet_size = RamPageSize * 2;

    struct viewstate state = {
        .clock = {
            .clock = {.rate = 10, .rate_factor = aldo_nes_cycle_factor()},
            .oldrate = Aldo_MinFps,
        },
        .running = true,
        .total_ramsheets = (int)aldo_nes_ram_size(emu->console) / sheet_size,
    };
    struct layout layout;
    printf("Stack Usage\nemu: %zu\nstate: %zu\nlayout: %zu\ntotal: %zu\n",
           sizeof *emu, sizeof state, sizeof layout,
           sizeof *emu + sizeof state + sizeof layout);
    init_ui(&layout, state.total_ramsheets);
    aldo_clock_start(&state.clock.clock);
    do {
        tick_start(&state, emu);
        handle_input(&state, emu);
        if (state.running) {
            aldo_nes_clock(emu->console, &state.clock.clock);
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

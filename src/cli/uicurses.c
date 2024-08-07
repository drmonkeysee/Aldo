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

enum scale_selection {
    SCL_CYCLE,
    SCL_FRAME,
};

enum ram_selection {
    RSEL_RAM,
    RSEL_VRAM,
    RSEL_PPU,
    RSEL_COUNT,
};

struct viewstate {
    struct runclock {
        struct cycleclock cyclock;
        double tickleft_ms;
    } clock;
    enum scale_selection scale;
    enum ram_selection ramselect;
    int oldrate, ramsheet, total_ramsheets;
    bool running;
};

static void tick_sleep(struct runclock *c)
{
    static const struct timespec vsync = {.tv_nsec = TSU_NS_PER_S / DisplayHz};

    struct timespec elapsed = timespec_elapsed(&c->cyclock.current);

    // NOTE: if elapsed nanoseconds is greater than vsync we're over
    // our time budget; if elapsed *seconds* is greater than vsync
    // we've BLOWN AWAY our time budget; either way don't sleep.
    if (elapsed.tv_nsec > vsync.tv_nsec || elapsed.tv_sec > vsync.tv_sec) {
        // NOTE: we've already blown the tick time, so convert everything
        // to milliseconds to make the math easier.
        c->tickleft_ms = timespec_to_ms(&vsync) - timespec_to_ms(&elapsed);
        return;
    }

    struct timespec tick_left = {
        .tv_nsec = vsync.tv_nsec - elapsed.tv_nsec,
    };
    c->tickleft_ms = timespec_to_ms(&tick_left);

    timespec_sleep(tick_left);
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
    if ((refreshdt += vs->clock.cyclock.ticktime_ms) >= refresh_interval_ms) {
        display_tickleft = vs->clock.tickleft_ms;
        display_ticktime = vs->clock.cyclock.ticktime_ms;
        refreshdt = 0;
    }

    mvwprintw(v->content, cursor_y++, 0, "Display Hz: %d (%.2f)", DisplayHz,
              (double)vs->clock.cyclock.ticks / vs->clock.cyclock.runtime);
    mvwprintw(v->content, cursor_y++, 0, "\u0394T: %.3f (%+.3f)",
              display_ticktime, display_tickleft);
    mvwprintw(v->content, cursor_y++, 0, "Ticks: %" PRIu64,
              vs->clock.cyclock.ticks);
    mvwprintw(v->content, cursor_y++, 0, "Runtime: %.3f",
              vs->clock.cyclock.runtime);
    mvwprintw(v->content, cursor_y++, 0, "Emutime: %.3f",
              vs->clock.cyclock.emutime);
    mvwprintw(v->content, cursor_y++, 0, "Frames: %" PRIu64,
              vs->clock.cyclock.frames);
    mvwprintw(v->content, cursor_y++, 0, "Cycles: %" PRIu64,
              vs->clock.cyclock.cycles);
    mvwprintw(v->content, cursor_y++, 0, "%s: %d",
              vs->scale == SCL_CYCLE
                ? "Cycles per Second"
                : "Frames per Second", vs->clock.cyclock.rate);
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
    drawtoggle(v, halt_label, !emu->snapshot.lines.ready);

    cursor_y += 2;
    enum csig_excmode mode = nes_mode(emu->console);
    mvwaddstr(v->content, cursor_y, 0, "Mode:");
    drawtoggle(v, " Sub ", mode == CSGM_SUBCYCLE);
    drawtoggle(v, " Cycle ", mode == CSGM_CYCLE);
    drawtoggle(v, " Step ", mode == CSGM_STEP);
    drawtoggle(v, " Run ", mode == CSGM_RUN);

    cursor_y += 2;
    mvwaddstr(v->content, cursor_y, 0, "Signal:");
    drawtoggle(v, " IRQ ", nes_probe(emu->console, CSGI_IRQ));
    drawtoggle(v, " NMI ", nes_probe(emu->console, CSGI_NMI));
    drawtoggle(v, " RST ", nes_probe(emu->console, CSGI_RST));

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
    static const struct haltexpr empty = {.cond = HLT_NONE};

    int cursor_y = 0;
    werase(v->content);
    mvwprintw(v->content, cursor_y++, 0, "Tracing: %s",
              emu->args->tron ? "On" : "Off");
    mvwaddstr(v->content, cursor_y++, 0, "Reset Override: ");
    int resetvector = debug_vector_override(emu->debugger);
    if (resetvector == NoResetVector) {
        waddstr(v->content, "None");
    } else {
        wprintw(v->content, "$%04X", resetvector);
    }
    const struct breakpoint *bp =
        debug_bp_at(emu->debugger, emu->snapshot.debugger.halted);
    char break_desc[HEXPR_FMT_SIZE];
    int err = haltexpr_desc(bp ? &bp->expr : &empty, break_desc);
    mvwprintw(v->content, cursor_y, 0, "Break: %s",
              err < 0 ? haltexpr_errstr(err) : break_desc);
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
    char fmtd[CART_FMT_SIZE];
    int err = cart_format_extname(emu->cart, fmtd);
    mvwprintw(v->content, ++cursor_y, 0, "Format: %s",
              err < 0 ? cart_errstr(err) : fmtd);
}

static void drawinstructions(const struct view *v, int h, int y,
                             const struct snapshot *snp)
{
    struct dis_instruction inst = {0};
    uint16_t addr = snp->datapath.current_instruction;
    char disassembly[DIS_INST_SIZE];
    for (int i = 0; i < h - y; ++i) {
        int result = dis_parsemem_inst(snp->mem.prglength,
                                       snp->mem.currprg,
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
    mvwprintw(v->content, h - y--, 0, "%04X: %02X %02X     RST",
              CPU_VECTOR_RST, lo, hi);
    int resetvector = debug_vector_override(emu->debugger);
    if (resetvector == NoResetVector) {
        wprintw(v->content, " $%04X", bytowr(lo, hi));
    } else {
        wprintw(v->content, " " HEXPR_RST_IND "$%04X", resetvector);
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

    drawinstructions(v, h, vector_offset, &emu->snapshot);
    drawvecs(v, h, w, vector_offset, emu);
}

static int drawregisters(const struct view *v, int cursor_y,
                         const struct snapshot *snp)
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
                     const struct snapshot *snp)
{
    int cursor_x = 0;
    mvwaddstr(v->content, cursor_y++, cursor_x, "N V - B D I Z C");
    for (size_t i = sizeof snp->cpu.status * 8; i > 0; --i) {
        mvwprintw(v->content, cursor_y, cursor_x, "%u",
                  byte_getbit(snp->cpu.status, i - 1));
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

static void drawdatapath(const struct view *v, int cursor_y, int w,
                         const struct snapshot *snp)
{
    static const int
        seph = 5, vsep1 = 7, vsep2 = 21, col1 = vsep1 + 2, col2 = vsep2 + 2;

    int line_x = (w / 4) + 1;
    draw_chip_vline(v, snp->lines.ready, cursor_y, line_x, 1, ArrowDown, -1,
                    "RDY");
    draw_chip_vline(v, snp->lines.sync, cursor_y, line_x * 2, 1, ArrowUp, -1,
                    "SYNC");
    draw_chip_vline(v, snp->lines.readwrite, cursor_y++, line_x * 3, 1,
                    ArrowUp, -1, "R/W\u0305");

    mvwhline(v->content, ++cursor_y, 0, 0, w);

    mvwvline(v->content, ++cursor_y, vsep1, 0, seph);
    mvwvline(v->content, cursor_y, vsep2, 0, seph);

    if (snp->datapath.jammed) {
        wattron(v->content, A_STANDOUT);
        mvwaddstr(v->content, cursor_y, col1, " JAMMED ");
        wattroff(v->content, A_STANDOUT);
    } else {
        char buf[DIS_DATAP_SIZE];
        int wlen = dis_datapath(snp, buf);
        const char *mnemonic = wlen < 0 ? dis_errstr(wlen) : buf;
        mvwaddstr(v->content, cursor_y, col1, mnemonic);
    }

    mvwprintw(v->content, ++cursor_y, col1, "adl: %02X",
              snp->datapath.addrlow_latch);

    mvwaddstr(v->content, ++cursor_y, 0, DArrowLeft);
    mvwprintw(v->content, cursor_y, 2, "%04X", snp->datapath.addressbus);
    mvwprintw(v->content, cursor_y, col1, "adh: %02X",
              snp->datapath.addrhigh_latch);
    if (snp->datapath.busfault) {
        mvwaddstr(v->content, cursor_y, col2, "FLT");
    } else {
        mvwprintw(v->content, cursor_y, col2, "%02X", snp->datapath.databus);
    }
    mvwaddstr(v->content, cursor_y, w - 1,
              snp->lines.readwrite ? DArrowLeft : DArrowRight);

    mvwprintw(v->content, ++cursor_y, col1, "adc: %02X",
              snp->datapath.addrcarry_latch);

    mvwprintw(v->content, ++cursor_y, col1, "%*sT%u", snp->datapath.exec_cycle,
              "", snp->datapath.exec_cycle);

    mvwhline(v->content, ++cursor_y, 0, 0, w);

    draw_interrupt_latch(v, snp->datapath.irq, cursor_y, line_x);
    draw_interrupt_latch(v, snp->datapath.nmi, cursor_y, line_x * 2);
    draw_interrupt_latch(v, snp->datapath.rst, cursor_y, line_x * 3);
    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    draw_chip_vline(v, snp->lines.irq, cursor_y, line_x, -1, ArrowUp, -1,
                    "I\u0305R\u0305Q\u0305");
    draw_chip_vline(v, snp->lines.nmi, cursor_y, line_x * 2, -1, ArrowUp, -1,
                    "N\u0305M\u0305I\u0305");
    draw_chip_vline(v, snp->lines.reset, cursor_y, line_x * 3, -1, ArrowUp, -1,
                    "R\u0305S\u0305T\u0305");
}

static void drawcpu(const struct view *v, const struct snapshot *snp)
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
                          int w, const struct snapshot *snp)
{
    uint8_t sel = snp->pdatapath.register_select;
    char sel_buf[4];
    sprintf(sel_buf, "%d%d%d", byte_getbit(sel, 2), byte_getbit(sel, 1),
            byte_getbit(sel, 0));
    draw_chip_vline(v, snp->plines.cpu_readwrite, cursor_y, line_x, 1,
                    ArrowDown, -1, sel_buf);
    draw_chip_vline(v, snp->plines.interrupt, cursor_y, line_x * 2, 1,
                    ArrowUp, -1, "I\u0305N\u0305T\u0305");
    draw_chip_vline(v, snp->plines.reset, cursor_y++, line_x * 3, 1, ArrowDown,
                    -1, "R\u0305S\u0305T\u0305");

    mvwhline(v->content, ++cursor_y, 0, 0, w);
    draw_interrupt_latch(v, snp->pdatapath.rst, cursor_y, line_x * 3);

    mvwprintw(v->content, cursor_y, line_x - 2, "[%s%02X]",
              snp->plines.cpu_readwrite ? DArrowUp : DArrowDown,
              snp->pdatapath.register_databus);
    return cursor_y;
}

static int draw_pregisters(const struct view *v, int cursor_y,
                           const struct snapshot *snp)
{
    uint8_t status = snp->ppu.status;
    mvwprintw(v->content, ++cursor_y, 0, "CONTROL: %02X  STATUS:  %d%d%d",
              snp->ppu.ctrl, byte_getbit(status, 7), byte_getbit(status, 6),
              byte_getbit(status, 5));
    mvwprintw(v->content, ++cursor_y, 0, "MASK:    %02X  OAMADDR: %02X",
              snp->ppu.mask, snp->ppu.oamaddr);
    return cursor_y + 1;
}

static void draw_scroll_addr(const struct view *v, int y, int x, char label,
                             uint16_t val)
{
    mvwprintw(v->content, y, x, "%c: %04X (%01d,%01d,%02d,%02d)", label, val,
              (val & 0x7000) >> 12, (val & 0xc00) >> 10, (val & 0x3e0) >> 5,
              val & 0x1f);
}

static int draw_pdatapath(const struct view *v, int cursor_y, int w,
                          const struct snapshot *snp)
{
    static const int seph = 5, vsep = 19, buscol = vsep + 2;

    mvwvline(v->content, ++cursor_y, vsep, 0, seph);

    draw_scroll_addr(v, cursor_y, 0, 'v', snp->pdatapath.scrolladdr);
    mvwprintw(v->content, cursor_y, buscol, "%04X", snp->pdatapath.addressbus);
    mvwaddstr(v->content, cursor_y, w - 1, DArrowRight);

    draw_scroll_addr(v, ++cursor_y, 0, 't', snp->pdatapath.tempaddr);
    draw_chip_hline(v, snp->plines.address_enable, cursor_y, buscol + 1, "ALE",
                    ArrowRight);

    mvwprintw(v->content, ++cursor_y, 0, "x: %02X (%02d)",
              snp->pdatapath.xfine, snp->pdatapath.xfine);
    draw_chip_hline(v, snp->plines.read, cursor_y, buscol + 3, "R\u0305",
                    ArrowRight);

    mvwprintw(v->content, ++cursor_y, 0, "c: %1d o: %1d w: %1d",
              snp->pdatapath.cv_pending, snp->pdatapath.oddframe,
              snp->pdatapath.writelatch);
    draw_chip_hline(v, snp->plines.write, cursor_y, buscol + 3, "W\u0305",
                    ArrowRight);

    mvwprintw(v->content, ++cursor_y, 0, "r: %02X", snp->pdatapath.readbuffer);
    if (snp->pdatapath.busfault) {
        mvwaddstr(v->content, cursor_y, buscol + 1, "FLT");
    } else {
        mvwprintw(v->content, cursor_y, buscol + 1, " %02X",
                  snp->pdatapath.databus);
    }
    // NOTE: write line does not signal when writing to palette ram so use the
    // read signal to determine direction of databus flow.
    mvwaddstr(v->content, cursor_y, w - 1,
              snp->plines.read ? DArrowRight : DArrowLeft);
    return cursor_y;
}

static void drawbottom_plines(const struct view *v, int cursor_y, int line_x,
                              const struct snapshot *snp)
{
    mvwprintw(v->content, cursor_y, line_x - 2, "[%sXX]", DArrowDown);

    char vbuf[10];
    sprintf(vbuf, "(%3d,%3d)", snp->pdatapath.line, snp->pdatapath.dot);
    // NOTE: jump 2 rows as interrupts are drawn direction first
    cursor_y += 2;
    draw_chip_vline(v, snp->plines.video_out, cursor_y, line_x, -1, ArrowDown,
                    -4, vbuf);
}

static void drawppu(const struct view *v, const struct snapshot *snp)
{
    int w = getmaxx(v->content), line_x = (w / 4) + 1;
    int cursor_y = drawtop_plines(v, 0, line_x, w, snp);
    cursor_y = draw_pregisters(v, cursor_y, snp);
    mvwhline(v->content, ++cursor_y, 0, 0, w);
    cursor_y = draw_pdatapath(v, cursor_y, w, snp);
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

    int cursor_y = draw_mempage(v, emu, emu->snapshot.mem.oam, RSEL_PPU,
                                start_x, 0, 0, 0, RamDim);
    wclrtoeol(v->content);
    cursor_y = draw_mempage(v, emu, emu->snapshot.mem.secondary_oam, RSEL_PPU,
                            start_x, cursor_y, 0, 0, 2);
    wclrtoeol(v->content);
    cursor_y = draw_mempage(v, emu, emu->snapshot.mem.palette, RSEL_PPU,
                            start_x, cursor_y, 0, 0x3f, 2);
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
    cycleclock_tickstart(&vs->clock.cyclock, !emu->snapshot.lines.ready);
}

static void tick_end(struct runclock *c)
{
    cycleclock_tickend(&c->cyclock);
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
    if (vs->scale == SCL_CYCLE) {
        min = MinCps;
        max = MaxCps;
    } else {
        min = MinFps;
        max = MaxFps;
    }
    applyrate(&vs->clock.cyclock.rate, adjustment, min, max);
}

static void selectrate(struct viewstate *vs)
{
    int prev = vs->oldrate;
    vs->oldrate = vs->clock.cyclock.rate;
    vs->clock.cyclock.rate = prev;
    vs->scale = !vs->scale;
    vs->clock.cyclock.rate_factor = vs->scale == SCL_CYCLE
                                    ? nes_cycle_factor()
                                    : nes_frame_factor();
}

static void handle_input(struct viewstate *vs, const struct emulator *emu)
{
    int input = getch();
    switch (input) {
    case ' ':
        nes_ready(emu->console, !emu->snapshot.lines.ready);
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
        selectrate(vs);
        break;
    case 'f':
        if (vs->ramselect != RSEL_PPU) {
            vs->ramsheet = (vs->ramsheet + 1) % vs->total_ramsheets;
        }
        break;
    case 'i':
        nes_set_probe(emu->console, CSGI_IRQ,
                      !nes_probe(emu->console, CSGI_IRQ));
        break;
    case 'm':
        nes_set_mode(emu->console, nes_mode(emu->console) + 1);
        break;
    case 'M':
        nes_set_mode(emu->console, nes_mode(emu->console) - 1);
        break;
    case 'n':
        nes_set_probe(emu->console, CSGI_NMI,
                      !nes_probe(emu->console, CSGI_NMI));
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
        nes_set_probe(emu->console, CSGI_RST,
                      !nes_probe(emu->console, CSGI_RST));
        break;
    }
}

static void emu_update(struct emulator *emu, struct viewstate *vs)
{
    nes_clock(emu->console, &vs->clock.cyclock);
    nes_snapshot(emu->console, &emu->snapshot);
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
        .clock.cyclock = {.rate = 10, .rate_factor = nes_cycle_factor()},
        .oldrate = MinFps,
        .running = true,
        .total_ramsheets = (int)nes_ram_size(emu->console) / sheet_size,
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

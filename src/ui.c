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

struct view {
    WINDOW *restrict win, *restrict content;
    PANEL *restrict outer, *restrict inner;
};

static const int RamViewPages = 4;

static struct view HwView;
static struct view ControlsView;
static struct view RomView;
static struct view RegistersView;
static struct view FlagsView;
static struct view DatapathView;
static struct view RamView;

static int CurrentRamViewPage;

static void drawhwtraits(uint64_t cycles)
{
    int cursor_y = 0;
    mvwaddstr(HwView.content, cursor_y++, 0, "FPS: N/A");
    mvwaddstr(HwView.content, cursor_y++, 0, "Master Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "CPU Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "PPU Clock: INF Hz");
    mvwprintw(HwView.content, cursor_y++, 0, "Cycles: %llu", cycles);
    mvwaddstr(HwView.content, cursor_y++, 0, "Cycles per Second: N/A");
    mvwaddstr(HwView.content, cursor_y++, 0, "Cycles per Frame: N/A");
}

static void drawcontrols(void)
{
    int cursor_y = 0;
    wattron(ControlsView.content, A_STANDOUT);
    mvwaddstr(ControlsView.content, cursor_y, 0, " HALT ");
    wattroff(ControlsView.content, A_STANDOUT);
    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Mode: ");
    wattron(ControlsView.content, A_STANDOUT);
    waddstr(ControlsView.content, " Cycle ");
    wattroff(ControlsView.content, A_STANDOUT);
    waddstr(ControlsView.content, " Step ");
    waddstr(ControlsView.content, " Run ");
    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y, 0, "Send: ");
    waddstr(ControlsView.content, " IRQ ");
    waddstr(ControlsView.content, " NMI ");
    waddstr(ControlsView.content, " RES ");
    mvwhline(ControlsView.content, ++cursor_y, 0, 0,
             getmaxx(ControlsView.content));
    cursor_y += 2;
    mvwaddstr(ControlsView.content, cursor_y++, 0, "FPS: [1-120]");
    mvwaddstr(ControlsView.content, cursor_y++, 0, "CPS: [1-100]");
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
    char disassembly[DIS_INST_SIZE];

    uint16_t addr = snapshot->cpu.currinst;
    // NOTE: on startup currinst is probably outside ROM range
    if (addr < CpuCartMinAddr) {
        mvwaddstr(RomView.content, 0, 0, "OUT OF ROM RANGE");
    } else {
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

    const int w = getmaxx(DatapathView.content), line_x = (w / 4) + 1;
    int cursor_y = 0;

    drawline(snapshot->lines.ready, cursor_y, line_x, 1, down, "RDY");
    drawline(snapshot->lines.sync, cursor_y, line_x * 2, 1, up, "SYNC");
    drawline(snapshot->lines.readwrite, cursor_y++, line_x * 3, 1, up, "R/W");

    mvwhline(DatapathView.content, ++cursor_y, 0, 0, w);

    mvwvline(DatapathView.content, ++cursor_y, vsep1, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep2, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep3, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep4, 0, 3);
    char buf[DIS_DATAP_SIZE];
    const int wlen = dis_datapath(snapshot, buf);
    // NOTE: only update datapath instruction display
    // if something useful was disasm'ed.
    if (wlen != 0) {
        const char *const mnemonic = wlen < 0 ? dis_errstr(wlen) : buf;
        mvwaddstr(DatapathView.content, cursor_y, vsep2 + 2, mnemonic);
    }

    mvwaddstr(DatapathView.content, ++cursor_y, 0, left);
    mvwprintw(DatapathView.content, cursor_y, vsep1 + 2, "$%04X",
              snapshot->cpu.addressbus);
    mvwprintw(DatapathView.content, cursor_y, vsep3 + 2, "$%02X",
              snapshot->cpu.databus);
    mvwaddstr(DatapathView.content, cursor_y, vsep4 + 1,
              snapshot->lines.readwrite ? left : right);

    mvwprintw(DatapathView.content, ++cursor_y, vsep2 + 2, "%*sT%u%*s",
              snapshot->cpu.exec_cycle, "", snapshot->cpu.exec_cycle,
              MaxCycleCount - snapshot->cpu.exec_cycle, "");

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
    RamView.content = newpad((h - 4) * RamViewPages, w - 4);
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
    pnoutrefresh(RamView.content, (ram_h - 4) * CurrentRamViewPage, 0,
                 ram_y + 2, ram_x + 2, ram_y + ram_h - 3, ram_x + ram_w - 2);
}

//
// Public Interface
//

void ui_init(void)
{
    static const int col1w = 32, col2w = 34, col3w = 35, col4w = 56, hwh = 12,
                     cpuh = 10, flagsh = 8, flagsw = 19, ramh = 37;

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
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

int ui_pollinput(void)
{
    return getch();
}

void ui_ram_next(void)
{
    CurrentRamViewPage = (CurrentRamViewPage + 1) % RamViewPages;
}

void ui_ram_prev(void)
{
    --CurrentRamViewPage;
    if (CurrentRamViewPage < 0) {
        CurrentRamViewPage = RamViewPages - 1;
    }
}

void ui_refresh(const struct console_state *snapshot, uint64_t cycles)
{
    drawhwtraits(cycles);
    drawcontrols();
    drawrom(snapshot);
    drawregister(snapshot);
    drawflags(snapshot);
    drawdatapath(snapshot);
    drawram(snapshot);

    update_panels();
    ramrefresh();
    doupdate();
}

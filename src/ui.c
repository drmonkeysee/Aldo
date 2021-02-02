//
//  ui.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/30/21.
//

#include "ui.h"

#include "asm.h"
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
    mvwaddstr(ControlsView.content, cursor_y++, 0, "Ram Page Next: n");
    mvwaddstr(ControlsView.content, cursor_y++, 0, "Ram Page Prev: b");
}

static void drawromerr(int diserr, int y)
{
    const char *err;
    switch (diserr) {
    case DIS_FMT_FAIL:
        err = "OUTPUT FAIL";
        break;
    case DIS_EOF:
        err = "UNEXPECTED EOF";
        break;
    default:
        err = "UNKNOWN ERR";
        break;
    }
    mvwaddstr(RomView.content, y, 0, err);
}

static void drawvecs(int h, int w, int y,
                     const struct console_state *snapshot)
{
    mvwhline(RomView.content, h - y--, 0, 0, w);

    uint8_t lo = snapshot->rom[NmiVector & CpuCartAddrMask],
            hi = snapshot->rom[(NmiVector + 1) & CpuCartAddrMask];
    mvwprintw(RomView.content, h - y--, 0, "$%04X: %02X %02X       NMI $%04X",
              NmiVector, lo, hi, lo | (hi << 8));

    lo = snapshot->rom[ResetVector & CpuCartAddrMask];
    hi = snapshot->rom[(ResetVector + 1) & CpuCartAddrMask];
    mvwprintw(RomView.content, h - y--, 0, "$%04X: %02X %02X       RES $%04X",
              ResetVector, lo, hi, lo | (hi << 8));

    lo = snapshot->rom[IrqVector & CpuCartAddrMask];
    hi = snapshot->rom[(IrqVector + 1) & CpuCartAddrMask];
    mvwprintw(RomView.content, h - y, 0, "$%04X: %02X %02X       IRQ $%04X",
              IrqVector, lo, hi, lo | (hi << 8));
}

static void drawrom(const struct console_state *snapshot)
{
    int h, w, vector_offset = 4;
    getmaxyx(RomView.content, h, w);
    char disassembly[DIS_INST_SIZE];

    uint16_t addr = snapshot->registers.program_counter;
    for (int i = 0, bytes = 0; i < h - vector_offset; ++i) {
        addr += bytes;
        const size_t cart_offset = addr & CpuCartAddrMask;
        bytes = dis_inst(addr, snapshot->rom + cart_offset,
                         ROM_SIZE - cart_offset, disassembly);
        if (bytes == 0) break;
        if (bytes < 0) {
            drawromerr(bytes, i);
            break;
        }
        mvwaddstr(RomView.content, i, 0, disassembly);
    }

    drawvecs(h, w, vector_offset, snapshot);
}

static void drawregister(const struct console_state *snapshot)
{
    int cursor_y = 0;
    mvwprintw(RegistersView.content, cursor_y++, 0, "PC: $%04X",
              snapshot->registers.program_counter);
    mvwprintw(RegistersView.content, cursor_y++, 0, "S:  $%02X",
              snapshot->registers.stack_pointer);
    mvwhline(RegistersView.content, cursor_y++, 0, 0,
             getmaxx(RegistersView.content));
    mvwprintw(RegistersView.content, cursor_y++, 0, "A:  $%02X",
              snapshot->registers.accum);
    mvwprintw(RegistersView.content, cursor_y++, 0, "X:  $%02X",
              snapshot->registers.xindex);
    mvwprintw(RegistersView.content, cursor_y++, 0, "Y:  $%02X",
              snapshot->registers.yindex);
}

static void drawflags(const struct console_state *snapshot)
{
    int cursor_x = 0, cursor_y = 0;
    mvwaddstr(FlagsView.content, cursor_y++, cursor_x, "7 6 5 4 3 2 1 0");
    mvwaddstr(FlagsView.content, cursor_y++, cursor_x, "N V - B D I Z C");
    mvwhline(FlagsView.content, cursor_y++, cursor_x, 0,
             getmaxx(FlagsView.content));
    for (size_t i = sizeof snapshot->registers.status * 8; i > 0; --i) {
        mvwprintw(FlagsView.content, cursor_y, cursor_x, "%u",
                  (snapshot->registers.status >> (i - 1)) & 1);
        cursor_x += 2;
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

    mvwaddstr(DatapathView.content, cursor_y, line_x - 1, "RDY");
    mvwaddstr(DatapathView.content, cursor_y, (line_x * 2) - 1, "SYNC");
    mvwaddstr(DatapathView.content, cursor_y++, (line_x * 3) - 1, "R/W");
    mvwaddstr(DatapathView.content, cursor_y, line_x, down);
    mvwaddstr(DatapathView.content, cursor_y, line_x * 2, up);
    mvwaddstr(DatapathView.content, cursor_y++, line_x * 3, up);

    mvwhline(DatapathView.content, cursor_y++, 0, 0, w);

    mvwvline(DatapathView.content, cursor_y, vsep1, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep2, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep3, 0, 3);
    mvwvline(DatapathView.content, cursor_y, vsep4, 0, 3);
    mvwaddstr(DatapathView.content, cursor_y, vsep2 + 2, "UNK ($0000)");

    mvwaddstr(DatapathView.content, ++cursor_y, 0, left);
    mvwaddstr(DatapathView.content, cursor_y, vsep1 + 2, "$1234");
    mvwaddstr(DatapathView.content, cursor_y, vsep3 + 2, "$42");
    mvwaddstr(DatapathView.content, cursor_y, vsep4 + 1, right);

    mvwaddstr(DatapathView.content, ++cursor_y, vsep2 + 2, "T0123456");

    mvwhline(DatapathView.content, ++cursor_y, 0, 0, w);

    mvwaddstr(DatapathView.content, ++cursor_y, line_x, up);
    mvwaddstr(DatapathView.content, cursor_y, line_x * 2, up);
    mvwaddstr(DatapathView.content, cursor_y++, line_x * 3, up);
    mvwaddstr(DatapathView.content, cursor_y, line_x - 1,
              "I\u0305R\u0305Q\u0305");
    mvwaddstr(DatapathView.content, cursor_y, (line_x * 2) - 1,
              "N\u0305M\u0305I\u0305");
    mvwaddstr(DatapathView.content, cursor_y, (line_x * 3) - 1,
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

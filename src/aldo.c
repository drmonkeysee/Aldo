//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "asm.h"
#include "emu/nes.h"
#include "emu/snapshot.h"
#include "emu/traits.h"

#include <ncurses.h>
#include <panel.h>

#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct view {
    WINDOW *restrict win, *restrict content;
    PANEL *restrict outer, *restrict inner;
};

static const int RamViewPages = 4;

static struct view HwView;
static struct view ControlsView;
static struct view RegistersView;
static struct view FlagsView;
static struct view RomView;
static struct view RamView;

static int CurrentRamViewPage;

static void ui_drawhwtraits(uint64_t cycles)
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

static void ui_drawcontrols(void)
{
    int cursor_y = 0;
    mvwaddstr(ControlsView.content, cursor_y++, 0, "Ram Page Next: n");
    mvwaddstr(ControlsView.content, cursor_y++, 0, "Ram Page Prev: b");
}

static void ui_drawregister(const struct console_state *snapshot)
{
    int cursor_y = 0;
    mvwprintw(RegistersView.content, cursor_y++, 0, "PC: $%04X",
              snapshot->program_counter);
    mvwprintw(RegistersView.content, cursor_y++, 0, "S:  $%02X",
              snapshot->stack_pointer);
    mvwhline(RegistersView.content, cursor_y++, 0, 0,
             getmaxx(RegistersView.content));
    mvwprintw(RegistersView.content, cursor_y++, 0, "A:  $%02X",
              snapshot->accum);
    mvwprintw(RegistersView.content, cursor_y++, 0, "X:  $%02X",
              snapshot->xindex);
    mvwprintw(RegistersView.content, cursor_y++, 0, "Y:  $%02X",
              snapshot->yindex);
}

static void ui_drawflags(const struct console_state *snapshot)
{
    int cursor_x = 0, cursor_y = 0;
    mvwaddstr(FlagsView.content, cursor_y++, cursor_x, "7 6 5 4 3 2 1 0");
    mvwaddstr(FlagsView.content, cursor_y++, cursor_x, "N V - B D I Z C");
    mvwhline(FlagsView.content, cursor_y++, cursor_x, 0,
             getmaxx(FlagsView.content));
    for (size_t i = sizeof snapshot->status * 8; i > 0; --i) {
        mvwprintw(FlagsView.content, cursor_y, cursor_x, "%u",
                  (snapshot->status >> (i - 1)) & 1);
        cursor_x += 2;
    }
}

static void ui_drawprogerr(int diserr, int y)
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

static void ui_drawvecs(int h, int w, int y,
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

static void ui_drawrom(const struct console_state *snapshot)
{
    int h, w, vector_offset = 4;
    getmaxyx(RomView.content, h, w);
    char disassembly[DIS_INST_SIZE];

    uint16_t addr = snapshot->program_counter;
    for (int i = 0, bytes = 0; i < h - vector_offset; ++i) {
        addr += bytes;
        const size_t cart_offset = addr & CpuCartAddrMask;
        bytes = dis_inst(addr, snapshot->rom + cart_offset,
                         ROM_SIZE - cart_offset, disassembly);
        if (bytes == 0) break;
        if (bytes < 0) {
            ui_drawprogerr(bytes, i);
            break;
        }
        mvwaddstr(RomView.content, i, 0, disassembly);
    }

    ui_drawvecs(h, w, vector_offset, snapshot);
}

static void ui_drawram(const struct console_state *snapshot)
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

static void ui_createwin(struct view *v, int h, int w, int y, int x,
                         const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->outer = new_panel(v->win);
    box(v->win, 0, 0);
    mvwaddstr(v->win, 0, 1, title);
}

static void ui_vinit(struct view *v, int h, int w, int y, int x,
                     const char *restrict title)
{
    ui_createwin(v, h, w, y, x, title);
    v->content = derwin(v->win, h - 4, w - 4, 2, 2);
    v->inner = new_panel(v->content);
}

static void ui_raminit(int h, int w, int y, int x,
                       const char *restrict title)
{
    ui_createwin(&RamView, h, w, y, x, title);
    RamView.content = newpad((h - 4) * RamViewPages, w - 4);
}

static void ui_vcleanup(struct view *v)
{
    del_panel(v->inner);
    delwin(v->content);
    del_panel(v->outer);
    delwin(v->win);
    *v = (struct view){0};
}

static void ui_init(void)
{
    static const int col1w = 32, col2w = 34, col3w = 19, col4w = 56, hwh = 12,
                        cpuh = 10, ramh = 37;
    ui_vinit(&HwView, hwh, col1w, 0, 0, "Hardware Traits");
    ui_vinit(&ControlsView, ramh - hwh, col1w, hwh, 0, "Controls");
    ui_vinit(&RomView, ramh, col2w, 0, col1w, "ROM");
    ui_vinit(&RegistersView, cpuh, col3w, 0, col1w + col2w, "Registers");
    ui_vinit(&FlagsView, 8, col3w, cpuh, col1w + col2w, "Flags");
    ui_raminit(ramh, col4w, 0, col1w + col2w + col3w, "RAM");
}

static void ui_ramrefresh(void)
{
    int ram_x, ram_y, ram_w, ram_h;
    getbegyx(RamView.win, ram_y, ram_x);
    getmaxyx(RamView.win, ram_h, ram_w);
    pnoutrefresh(RamView.content, (ram_h - 4) * CurrentRamViewPage, 0,
                 ram_y + 2, ram_x + 2, ram_h - 3, ram_x + ram_w - 2);
}

static void ui_refresh(const struct console_state *snapshot, uint64_t cycles)
{
    ui_drawhwtraits(cycles);
    ui_drawcontrols();
    ui_drawregister(snapshot);
    ui_drawflags(snapshot);
    ui_drawrom(snapshot);
    ui_drawram(snapshot);

    update_panels();
    ui_ramrefresh();
    doupdate();
}

static void ui_cleanup(void)
{
    ui_vcleanup(&RamView);
    ui_vcleanup(&RomView);
    ui_vcleanup(&FlagsView);
    ui_vcleanup(&RegistersView);
    ui_vcleanup(&ControlsView);
    ui_vcleanup(&HwView);
}

//
// Public Interface
//

int aldo_run(void)
{
    puts("Aldo starting...");

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);
    curs_set(0);

    nes *console = nes_new();

    ui_init();
    bool running = true;
    uint64_t total_cycles = 0;
    struct console_state snapshot;
    uint8_t test_prog[] = { 0xea, 0xea, 0xea }; // just a bunch of NOPs
    nes_powerup(console, sizeof test_prog, test_prog);
    do {
        const int c = getch();
        switch (c) {
        case ' ':
            total_cycles += nes_step(console);
            break;
        case 'b':
            --CurrentRamViewPage;
            if (CurrentRamViewPage < 0) {
                CurrentRamViewPage = RamViewPages - 1;
            }
            break;
        case 'n':
            CurrentRamViewPage = (CurrentRamViewPage + 1) % RamViewPages;
            break;
        case 'q':
            running = false;
            break;
        }
        if (running) {
            nes_snapshot(console, &snapshot);
            ui_refresh(&snapshot, total_cycles);
        }
    } while (running);
    ui_cleanup();

    nes_free(console);
    console = NULL;

    endwin();

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}

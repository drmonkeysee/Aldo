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
static struct view CpuView;
static struct view FlagsView;
static struct view ProgView;
static struct view RamView;

static int CurrentRamViewPage;

static void ui_drawhwtraits(uint64_t cycles)
{
    int cursor_y = 0;
    mvwaddstr(HwView.content, cursor_y++, 0, "FPS: N/A");
    mvwaddstr(HwView.content, cursor_y++, 0, "Master Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "CPU Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "PPU Clock: INF Hz");
    mvwprintw(HwView.content, cursor_y++, 0, "Cycle Count: %u", cycles);
    mvwaddstr(HwView.content, cursor_y++, 0, "Freq Multiplier: 1x");
}

static void ui_drawcpu(const struct console_state *snapshot)
{
    int cursor_y = 0;
    mvwprintw(CpuView.content, cursor_y++, 0, "PC: $%04X",
              snapshot->program_counter);
    mvwprintw(CpuView.content, cursor_y++, 0, "S:  $%02X",
              snapshot->stack_pointer);
    mvwhline(CpuView.content, cursor_y++, 0, 0, getmaxx(CpuView.content));
    mvwprintw(CpuView.content, cursor_y++, 0, "A:  $%02X", snapshot->accum);
    mvwprintw(CpuView.content, cursor_y++, 0, "X:  $%02X", snapshot->xindex);
    mvwprintw(CpuView.content, cursor_y++, 0, "Y:  $%02X", snapshot->yindex);
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
    mvwaddstr(ProgView.content, y, 0, err);
}

static void ui_drawvecs(int h, int w, int y,
                        const struct console_state *snapshot)
{
    mvwhline(ProgView.content, h - y--, 0, 0, w);

    uint8_t lo = snapshot->cart[NmiVector & CpuCartAddrMask],
            hi = snapshot->cart[(NmiVector + 1) & CpuCartAddrMask];
    mvwprintw(ProgView.content, h - y--, 0, "$%04X: %02X %02X       NMI $%04X",
              NmiVector, lo, hi, lo | (hi << 8));

    lo = snapshot->cart[ResetVector & CpuCartAddrMask];
    hi = snapshot->cart[(ResetVector + 1) & CpuCartAddrMask];
    mvwprintw(ProgView.content, h - y--, 0, "$%04X: %02X %02X       RST $%04X",
              ResetVector, lo, hi, lo | (hi << 8));

    lo = snapshot->cart[IrqVector & CpuCartAddrMask];
    hi = snapshot->cart[(IrqVector + 1) & CpuCartAddrMask];
    mvwprintw(ProgView.content, h - y, 0, "$%04X: %02X %02X       IRQ $%04X",
              IrqVector, lo, hi, lo | (hi << 8));
}

static void ui_drawprog(const struct console_state *snapshot)
{
    int h, w, vector_offset = 4;
    getmaxyx(ProgView.content, h, w);
    char disassembly[DIS_INST_SIZE];

    uint16_t addr = snapshot->program_counter;
    for (int i = 0, bytes = 0; i < h - vector_offset; ++i) {
        addr += bytes;
        const size_t cart_offset = addr & CpuCartAddrMask;
        bytes = dis_inst(addr, snapshot->cart + cart_offset,
                         ROM_SIZE - cart_offset, disassembly);
        if (bytes == 0) break;
        if (bytes < 0) {
            ui_drawprogerr(bytes, i);
            break;
        }
        mvwaddstr(ProgView.content, i, 0, disassembly);
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
    ui_vinit(&HwView, 12, 24, 0, 0, "Hardware Traits");
    ui_vinit(&CpuView, 10, 17, 13, 0, "CPU");
    ui_vinit(&FlagsView, 8, 19, 24, 0, "Flags");
    ui_vinit(&ProgView, 37, 34, 0, 25, "Program");
    ui_raminit(37, 56, 0, 60, "RAM");
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
    ui_drawcpu(snapshot);
    ui_drawflags(snapshot);
    ui_drawprog(snapshot);
    ui_drawram(snapshot);

    update_panels();
    ui_ramrefresh();
    doupdate();
}

static void ui_cleanup(void)
{
    ui_vcleanup(&RamView);
    ui_vcleanup(&ProgView);
    ui_vcleanup(&FlagsView);
    ui_vcleanup(&CpuView);
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
    uint64_t instruction_cycles = 0;
    struct console_state snapshot;
    uint8_t test_prog[] = { 0xea, 0xea, 0xea }; // just a bunch of NOPs
    nes_powerup(console, sizeof test_prog, test_prog);
    do {
        const int c = getch();
        switch (c) {
        case ' ':
            instruction_cycles = nes_step(console);
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
            ui_refresh(&snapshot, instruction_cycles);
        }
    } while (running);
    ui_cleanup();

    nes_free(console);
    console = NULL;

    endwin();

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}

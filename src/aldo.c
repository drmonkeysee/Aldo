//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "emu/nes.h"

#include <ncurses.h>
#include <panel.h>

#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct view {
    WINDOW *win, *content;
    PANEL *outer, *inner;
};

static const int RamViewBlocks = 4;

static struct view DebugView;
static struct view HwView;
static struct view CpuView;
static struct view FlagsView;
static struct view RamView;

static int RamViewBlock;

static void ui_drawhwtraits(const struct view *v)
{
    int cursor_y = 0;
    mvwaddstr(v->content, cursor_y++, 0, "FPS: N/A");
    mvwaddstr(v->content, cursor_y++, 0, "Clock: INF Hz");
    mvwaddstr(v->content, cursor_y++, 0, "Cycle Count: 1");
    mvwaddstr(v->content, cursor_y++, 0, "Freq Multiplier: 1x");
}

static void ui_drawcpu(const struct view *v)
{
    int cursor_y = 0;
    mvwaddstr(v->content, cursor_y++, 0, "PC: $FFFF");
    mvwaddstr(v->content, cursor_y++, 0, "SP: $FF");
    mvwhline(v->content, cursor_y++, 0, 0, getmaxx(v->content));
    mvwaddstr(v->content, cursor_y++, 0, "A:  $FF");
    mvwaddstr(v->content, cursor_y++, 0, "X:  $FF");
    mvwaddstr(v->content, cursor_y++, 0, "Y:  $FF");
}

static void ui_drawflags(const struct view *v)
{
    int cursor_y = 0;
    mvwaddstr(v->content, cursor_y++, 0, "(0) C: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(1) Z: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(2) I: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(3) D: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(4) B: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(5) -: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(6) V: $0");
    mvwaddstr(v->content, cursor_y++, 0, "(7) N: $0");
}

static void ui_drawram(const struct view *v)
{
    static const int start_x = 6, col_width = 4;
    int cursor_x = start_x, cursor_y = 0;
    mvwvline(v->content, 0, start_x - 2, 0, getmaxy(v->content));
    for (unsigned int page = 0; page < 8; ++page) {
        for (unsigned int upper = 0; upper < 0x10; ++upper) {
            mvwprintw(v->content, cursor_y, 0, "$%02X", page);
            for (unsigned int lower = 0; lower < 0x10; ++lower) {
                mvwprintw(v->content, cursor_y, cursor_x, "$%X%X", upper,
                          lower);
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

static void ui_vinit(struct view *v, int h, int w, int y, int x,
                     const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->outer = new_panel(v->win);
    box(v->win, 0, 0);
    mvwaddstr(v->win, 0, 1, title);
    v->content = derwin(v->win, h - 4, w - 4, 2, 2);
    v->inner = new_panel(v->content);
}

static void ui_raminit(struct view *v, int h, int w, int y, int x,
                       const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->outer = new_panel(v->win);
    box(v->win, 0, 0);
    mvwaddstr(v->win, 0, 1, title);
    v->content = newpad((h - 4) * RamViewBlocks, w - 4);
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
    ui_vinit(&DebugView, 37, 25, 0, 25, "Debug");
    scrollok(DebugView.content, true);
    ui_vinit(&HwView, 12, 24, 0, 0, "Hardware Traits");
    ui_vinit(&CpuView, 10, 17, 13, 0, "CPU");
    ui_vinit(&FlagsView, 12, 17, 24, 0, "Flags");
    ui_raminit(&RamView, 37, 73, 0, 51, "RAM");
    ui_drawhwtraits(&HwView);
    ui_drawcpu(&CpuView);
    ui_drawflags(&FlagsView);
    ui_drawram(&RamView);
}

static void ui_ramrefresh(void)
{
    int ram_x, ram_y, ram_w, ram_h;
    getbegyx(RamView.win, ram_y, ram_x);
    getmaxyx(RamView.win, ram_h, ram_w);
    pnoutrefresh(RamView.content, (ram_h - 4) * RamViewBlock, 0, ram_y + 2,
                 ram_x + 2, ram_h - 3, ram_x + ram_w - 2);
}

static void ui_refresh(void)
{
    update_panels();
    ui_ramrefresh();
    doupdate();
}

static void ui_cleanup(void)
{
    ui_vcleanup(&RamView);
    ui_vcleanup(&FlagsView);
    ui_vcleanup(&CpuView);
    ui_vcleanup(&HwView);
    ui_vcleanup(&DebugView);
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

    ui_init();
    bool running = true;
    int debug_cursor_y = -1;
    const int visible_debugrows = getmaxy(DebugView.content) - 1;
    do {
        const int c = getch();
        switch (c) {
        case ' ':
            if (debug_cursor_y < visible_debugrows) {
                ++debug_cursor_y;
            } else {
                scroll(DebugView.content);
            }
            mvwprintw(DebugView.content, debug_cursor_y, 0, "NES number: %X",
                      (unsigned int)nes_rand());
            break;
        case 'b':
            --RamViewBlock;
            if (RamViewBlock < 0) {
                RamViewBlock = RamViewBlocks - 1;
            }
            break;
        case 'n':
            RamViewBlock = (RamViewBlock + 1) % RamViewBlocks;
            break;
        case 'q':
            running = false;
            break;
        }
        ui_refresh();
    } while (running);
    ui_cleanup();

    endwin();

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}

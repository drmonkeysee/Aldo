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

static const int RamViewPages = 4;

static struct view DebugView;
static struct view HwView;
static struct view CpuView;
static struct view FlagsView;
static struct view RamView;

static int CurrentRamViewPage;

static void ui_drawhwtraits(void)
{
    int cursor_y = 0;
    mvwaddstr(HwView.content, cursor_y++, 0, "FPS: N/A");
    mvwaddstr(HwView.content, cursor_y++, 0, "Clock: INF Hz");
    mvwaddstr(HwView.content, cursor_y++, 0, "Cycle Count: 1");
    mvwaddstr(HwView.content, cursor_y++, 0, "Freq Multiplier: 1x");
}

static void ui_drawcpu(void)
{
    int cursor_y = 0;
    mvwaddstr(CpuView.content, cursor_y++, 0, "PC: $FFFF");
    mvwaddstr(CpuView.content, cursor_y++, 0, "SP: $FF");
    mvwhline(CpuView.content, cursor_y++, 0, 0, getmaxx(CpuView.content));
    mvwaddstr(CpuView.content, cursor_y++, 0, "A:  $FF");
    mvwaddstr(CpuView.content, cursor_y++, 0, "X:  $FF");
    mvwaddstr(CpuView.content, cursor_y++, 0, "Y:  $FF");
}

static void ui_drawflags(void)
{
    int cursor_y = 0;
    mvwaddstr(FlagsView.content, cursor_y++, 0, "7 6 5 4 3 2 1 0");
    mvwaddstr(FlagsView.content, cursor_y++, 0, "N V - B D I Z C");
    mvwhline(FlagsView.content, cursor_y++, 0, 0, getmaxx(FlagsView.content));
    mvwaddstr(FlagsView.content, cursor_y++, 0, "0 0 0 0 0 0 0 0");
}

static void ui_drawram(void)
{
    static const int start_x = 6, col_width = 4;
    int cursor_x = start_x, cursor_y = 0;
    mvwvline(RamView.content, 0, start_x - 2, 0, getmaxy(RamView.content));
    for (unsigned int page = 0; page < 8; ++page) {
        for (unsigned int upper = 0; upper < 0x10; ++upper) {
            mvwprintw(RamView.content, cursor_y, 0, "$%02X", page);
            for (unsigned int lower = 0; lower < 0x10; ++lower) {
                mvwprintw(RamView.content, cursor_y, cursor_x, "$%X%X", upper,
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
    ui_vinit(&DebugView, 37, 25, 0, 25, "Debug");
    scrollok(DebugView.content, true);
    ui_vinit(&HwView, 12, 24, 0, 0, "Hardware Traits");
    ui_vinit(&CpuView, 10, 17, 13, 0, "CPU");
    ui_vinit(&FlagsView, 8, 19, 24, 0, "Flags");
    ui_raminit(37, 73, 0, 51, "RAM");
    ui_drawhwtraits();
    ui_drawcpu();
    ui_drawflags();
    ui_drawram();
}

static void ui_ramrefresh(void)
{
    int ram_x, ram_y, ram_w, ram_h;
    getbegyx(RamView.win, ram_y, ram_x);
    getmaxyx(RamView.win, ram_h, ram_w);
    pnoutrefresh(RamView.content, (ram_h - 4) * CurrentRamViewPage, 0, ram_y + 2,
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
        ui_refresh();
    } while (running);
    ui_cleanup();

    endwin();

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}

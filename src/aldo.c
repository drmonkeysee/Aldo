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

static struct view DebugView;
static struct view HwView;
static struct view CpuView;
static struct view RamView;

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
    int cursor_y = 1;
    mvwaddstr(v->content, cursor_y++, 0, "PC: $FFFF");
    mvwaddstr(v->content, cursor_y++, 0, "SP: $FF");
    mvwhline(v->content, cursor_y++, 0, 0, getmaxx(v->content));
    mvwaddstr(v->content, cursor_y++, 0, "A:  $FF");
    mvwaddstr(v->content, cursor_y++, 0, "X:  $FF");
    mvwaddstr(v->content, cursor_y++, 0, "Y:  $FF");
}

static void ui_drawram(const struct view *v)
{
    static const int start_x = 6, col_width = 4;
    int cursor_x = start_x, cursor_y = 0;
    mvwvline(v->content, 0, start_x - 2, 0, getmaxy(v->content));
    for (unsigned int page = 0; page < 2; ++page) {
        for (unsigned int msb = 0; msb < 0x10; ++msb) {
            mvwprintw(v->content, cursor_y, 0, "$%02X", page);
            for (unsigned int lsb = 0; lsb < 0x10; ++lsb) {
                mvwprintw(v->content, cursor_y, cursor_x, "$%X%X", msb, lsb);
                cursor_x += col_width;
            }
            cursor_x = start_x;
            ++cursor_y;
        }
        ++cursor_y;
    }
}

static void ui_vinit(struct view *v, int h, int w, int y, int x,
                     const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->outer = new_panel(v->win);
    box(v->win, 0, 0);
    mvwaddstr(v->win, 0, 1, title);
    v->content = derwin(v->win, h - 2, w - 2, 1, 1);
    v->inner = new_panel(v->content);
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
    ui_vinit(&DebugView, 35, 24, 0, 23, "Debug");
    scrollok(DebugView.content, true);
    ui_vinit(&HwView, 10, 22, 0, 0, "Hardware Traits");
    ui_vinit(&CpuView, 10, 15, 25, 0, "CPU");
    ui_vinit(&RamView, 35, 71, 0, 48, "RAM");
    ui_drawhwtraits(&HwView);
    ui_drawcpu(&CpuView);
    ui_drawram(&RamView);
}

static void ui_refresh(void)
{
    update_panels();
    doupdate();
}

static void ui_cleanup(void)
{
    ui_vcleanup(&RamView);
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

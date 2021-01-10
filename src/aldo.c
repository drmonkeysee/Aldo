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
#include <stdio.h>
#include <stdlib.h>

struct view {
    WINDOW *win;
    PANEL *panel;
};

static struct view DebugView;
static struct view CpuView;

static void ui_vinit(struct view *v, int h, int w, int y, int x,
                     const char *restrict title)
{
    v->win = newwin(h, w, y, x);
    v->panel = new_panel(v->win);
    box(v->win, 0, 0);
    mvwaddstr(v->win, 0, 1, title);
}

static void ui_vcleanup(struct view *v)
{
    del_panel(v->panel);
    delwin(v->win);
    *v = (struct view){0};
}

static void ui_init(void)
{
    ui_vinit(&DebugView, 40, 22, 1, 1, "Debug");
    ui_vinit(&CpuView, 10, 10, 5, 30, "CPU");
}

static void ui_refresh(void)
{
    update_panels();
    doupdate();
}

static void ui_cleanup(void)
{
    ui_vcleanup(&CpuView);
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
    int debug_cursor_y = 1;
    do {
        const int c = getch();
        switch (c) {
        case ' ':
            mvwprintw(DebugView.win, debug_cursor_y++, 1, "NES number: %X",
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

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
    WINDOW *win;
    PANEL *panel;
};

struct debugview {
    struct view v;
    WINDOW *content;
    PANEL *panel;
};

static struct debugview DebugView;
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
    ui_vinit(&DebugView.v, 40, 24, 1, 1, "Debug");
    DebugView.content = derwin(DebugView.v.win, 38, 22, 1, 1);
    DebugView.panel = new_panel(DebugView.content);
    scrollok(DebugView.content, true);
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

    del_panel(DebugView.panel);
    DebugView.panel = NULL;
    delwin(DebugView.content);
    DebugView.content = NULL;
    ui_vcleanup(&DebugView.v);
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
    do {
        const int c = getch();
        switch (c) {
        case ' ':
            if (debug_cursor_y < 37) {
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

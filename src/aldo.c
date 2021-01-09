//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "emu/nes.h"

#include <ncurses.h>

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int aldo_run(void)
{
    puts("Aldo starting...");

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true);

    addstr("Hello from Aldo!\n\n");
    bool running = true;
    do {
        const int c = getch();
        switch (c) {
        case ' ':
            printw("NES number: %x\n", (unsigned int)nes_rand());
            break;
        case 'q':
            running = false;
            break;
        }
    } while (running);

    endwin();

    puts("Aldo stopping...");
    return EXIT_SUCCESS;
}

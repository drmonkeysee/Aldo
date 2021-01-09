//
//  aldo.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/8/21.
//

#include "aldo.h"

#include "emu/nes.h"

#include <stdio.h>
#include <stdlib.h>

int aldo_run(void)
{
    puts("Hello from Aldo!");
    nes_hello();
    return EXIT_SUCCESS;
}

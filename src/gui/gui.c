//
//  gui.c
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#include "gui.h"

#include "argparse.h"
#include "cliargs.h"
#include "snapshot.h"
#include "ui.h"
#include "uisdl.hpp"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

static int init_ui(const struct cliargs *args,
                   const struct gui_platform *platform,
                   ui_loop **loop)
{
    return args->batch
            ? ui_batch_init(args, loop)
            : ui_sdl_init(args, platform, loop);
}

static int sdl_demo(struct cliargs *args, const struct gui_platform *platform)
{
    int result = EXIT_SUCCESS;
    struct console_state snapshot = {0};

    ui_loop *loop;
    errno = 0;
    const int err = init_ui(args, platform, &loop);
    if (err < 0) {
        fprintf(stderr, "UI init failure (%d): %s\n", err, ui_errstr(err));
        if (err == UI_ERR_ERNO) {
            perror("UI System Error");
        }
        result = EXIT_FAILURE;
        goto exit_console;
    }

    loop(NULL, &snapshot);
exit_console:
    snapshot_clear(&snapshot);
    return result;
}

//
// Public Interface
//

int gui_run(int argc, char *argv[argc+1],
            const struct gui_platform *restrict platform)
{
    assert(platform != NULL);

    struct cliargs args;
    if (!argparse_parse(&args, argc, argv)) return EXIT_FAILURE;

    const int result = sdl_demo(&args, platform);
    argparse_cleanup(&args);
    return result;
}

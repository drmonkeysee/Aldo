//
//  argparse.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/23/21.
//

#ifndef Aldo_cli_argparse_h
#define Aldo_cli_argparse_h

#include "cliargs.h"

#include <stdbool.h>

bool argparse_parse(struct cliargs *restrict args, int argc,
                    char *argv[argc+1]);
void argparse_usage(const char *me);
void argparse_version(void);
void argparse_cleanup(struct cliargs *args);

#endif

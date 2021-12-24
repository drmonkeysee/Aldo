//
//  argparse.h
//  Aldo
//
//  Created by Brandon Stansbury on 12/23/21.
//

#ifndef Aldo_argparse_h
#define Aldo_argparse_h

#include "control.h"

#include <stdbool.h>

bool argparse_parse(struct control *restrict appstate, int argc,
                    char *argv[argc+1]);
void argparse_usage(const char *me);
void argparse_version(void);

#endif

//
//  cliargs.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/14/21.
//

#ifndef Aldo_cli_cliargs_h
#define Aldo_cli_cliargs_h

#include <stdbool.h>

struct haltarg {
    const char *expr;       // Non-owning Pointer
    struct haltarg *next;
};

struct cliargs {
    struct haltarg *haltlist;
    const char *cartfile, *chrdecode_prefix, *me;   // Non-owning Pointers
    int chrscale, resetvector;
    bool
        batch, bcdsupport, chrdecode, disassemble, help, info, tron, verbose,
        version, zeroram;
};

#endif

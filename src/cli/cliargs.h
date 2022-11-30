//
//  cliargs.h
//  Aldo
//
//  Created by Brandon Stansbury on 2/14/21.
//

#ifndef Aldo_cli_cliargs_h
#define Aldo_cli_cliargs_h

#include <stdbool.h>

struct cliargs {
    struct haltarg {
        const char *expr;               // Non-owning Pointer
        struct haltarg *next;
    } *haltlist;
    const char                          // Non-owning Pointers
        *cartfilepath, *cartfilename,
        *chrdecode_prefix, *me;
    int chrscale, resetvector;
    bool
        batch, bcdsupport, chrdecode, disassemble, help, info, tron, verbose,
        version, zeroram;
};

#endif

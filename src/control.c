//
//  control.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/14/21.
//

#include "control.h"

#include <assert.h>
#include <string.h>

const int MinCps = 1, MaxCps = 1000, RamSheets = 4;

const char *ctrl_cartfilename(const char *cartfile)
{
    assert(cartfile != NULL);

    const char *const last_slash = strrchr(cartfile, '/');
    return last_slash ? last_slash + 1 : cartfile;
}

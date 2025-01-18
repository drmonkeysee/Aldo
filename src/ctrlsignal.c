//
//  ctrlsignal.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/17/25.
//

#include "ctrlsignal.h"

const char *aldo_ntmirror_name(enum aldo_ntmirror m)
{
    switch (m) {
#define X(s, n) case ALDO_##s: return n;
        ALDO_NTMIRROR_X
#undef X
    default:
        return "UNKNOWN INES NT MIRROR";
    }
}

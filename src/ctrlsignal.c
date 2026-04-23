//
//  ctrlsignal.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/17/25.
//

#include "ctrlsignal.h"

extern inline enum aldo_trivalue aldo_lnptotriv(bool hi, bool lo);
extern inline void aldo_trivtolnp(enum aldo_trivalue v, bool *restrict hi,
                                  bool *restrict lo),
                   aldo_booltolnp(bool v, bool *restrict hi, bool *restrict lo);

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

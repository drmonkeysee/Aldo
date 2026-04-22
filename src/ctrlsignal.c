//
//  ctrlsignal.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/17/25.
//

#include "ctrlsignal.h"

extern inline enum aldo_probe_value aldo_lnptoprb(bool hi, bool lo);
extern inline void aldo_prbtolnp(enum aldo_probe_value v, bool *restrict hi,
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

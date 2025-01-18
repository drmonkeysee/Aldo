//
//  ctrlsignal.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/17/22.
//

#ifndef Aldo_ctrlsignal_h
#define Aldo_ctrlsignal_h

enum aldo_execmode {
    ALDO_EXC_SUBCYCLE,
    ALDO_EXC_CYCLE,
    ALDO_EXC_STEP,
    ALDO_EXC_RUN,
    ALDO_EXC_COUNT,
};

enum aldo_interrupt {
    ALDO_INT_IRQ,
    ALDO_INT_NMI,
    ALDO_INT_RST,
};

enum aldo_sigstate {
    ALDO_SIG_CLEAR,
    ALDO_SIG_DETECTED,
    ALDO_SIG_PENDING,
    ALDO_SIG_COMMITTED,
    ALDO_SIG_SERVICED,
};

// TODO: add additional mirror types as we expand mapper support
// X(symbol, name)
#define ALDO_NTMIRROR_X \
X(NTM_HORIZONTAL, "Horizontal") \
X(NTM_VERTICAL, "Vertical") \
X(NTM_1SCREEN, "Single-Screen") \
X(NTM_4SCREEN, "4-Screen VRAM") \
X(NTM_OTHER, "Mapper-Specific")

enum aldo_ntmirror {
#define X(s, n) ALDO_##s,
    ALDO_NTMIRROR_X
#undef X
};

#include "bridgeopen.h"
aldo_export
const char *aldo_ntmirror_name(enum aldo_ntmirror m) aldo_nothrow;
#include "bridgeclose.h"

#endif

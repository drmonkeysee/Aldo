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

#endif

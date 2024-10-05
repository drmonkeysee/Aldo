//
//  ctrlsignal.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/17/22.
//

#ifndef Aldo_ctrlsignal_h
#define Aldo_ctrlsignal_h

enum aldo_execmode {
    CSGM_SUBCYCLE,
    CSGM_CYCLE,
    CSGM_STEP,
    CSGM_RUN,
    CSGM_COUNT,
};

enum aldo_interrupt {
    CSGI_IRQ,
    CSGI_NMI,
    CSGI_RST,
};

enum aldo_sigstate {
    CSGS_CLEAR,
    CSGS_DETECTED,
    CSGS_PENDING,
    CSGS_COMMITTED,
    CSGS_SERVICED,
};

#endif

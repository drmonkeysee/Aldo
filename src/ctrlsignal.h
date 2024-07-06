//
//  ctrlsignal.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/17/22.
//

#ifndef Aldo_ctrlsignal_h
#define Aldo_ctrlsignal_h

enum csig_excmode {
    CSGM_CYCLE,
    CSGM_STEP,
    CSGM_RUN,
    CSGM_MODECOUNT,
};

enum csig_interrupt {
    CSGI_IRQ,
    CSGI_NMI,
    CSGI_RST,
};

enum csig_state {
    CSGS_CLEAR,
    CSGS_DETECTED,
    CSGS_PENDING,
    CSGS_COMMITTED,
    CSGS_SERVICED,
};

#endif

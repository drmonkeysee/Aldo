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

// Probe lines used to directly test emulator functionality
enum aldo_probe {
    ALDO_PRB_IRQ,       // CPU IRQ Line
    ALDO_PRB_NMI,       // CPU NMI Line
    ALDO_PRB_RDY,       // CPU READY Line
    ALDO_PRB_RST,       // CPU RESET Line

    ALDO_PRB_PPU_GRAY,  // PPU Grayscale
    ALDO_PRB_PPU_TLFT,  // PPU Background Left-Column Enabled
    ALDO_PRB_PPU_SLFT,  // PPU Sprite Left-Column Enabled
    ALDO_PRB_PPU_TILE,  // PPU Background Rendering
    ALDO_PRB_PPU_SPR,   // PPU Sprite Rendering
    ALDO_PRB_PPU_RED,   // PPU Red Emphasis
    ALDO_PRB_PPU_GRN,   // PPU Green Emphasis
    ALDO_PRB_PPU_BLU,   // PPU Blue Emphasis
};

/*
 * A device is free to translate tri-value semantics in counter-intuitive ways
 * based on the most useful test behavior to model;
 *
 * CPU IRQ:
 *  - null/false = irq line pulled high = no active interrupt
 *      - other hardware connected to IRQ line can still initiate interrupts
 *  - true = irq line pulled low = active interrupt
 *
 * PPU Grayscale:
 *  - null = no effect, game determines flag
 *  - false = game cannot turn on grayscale
 *  - true = game cannot turn off grayscale
 *
 * note in the above the IRQ probe acts as a peer device on the interrupt line
 * and can be overridden by other devices on the same line; the PPU probe is a
 * hard override, when active the PPU cannot affect its own register lines.
 */
enum aldo_trivalue {
    ALDO_TRIV_NULL = -1,
    ALDO_TRIV_FALSE,
    ALDO_TRIV_TRUE,
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
// TODO: are these export or internal
// Line-Pair to Tri-Value
aldo_export
inline enum aldo_trivalue aldo_lnptotriv(bool hi, bool lo) aldo_nothrow
{
    if (hi) return ALDO_TRIV_TRUE;
    if (lo) return ALDO_TRIV_FALSE;
    return ALDO_TRIV_NULL;
}

// Tri-Value to Line-Pair
aldo_export
inline void aldo_trivtolnp(enum aldo_trivalue v, bool *aldo_noalias hi,
                           bool *aldo_noalias lo) aldo_nothrow
{
    *hi = v == ALDO_TRIV_TRUE;
    *lo = v == ALDO_TRIV_FALSE;
}

// Boolean to Line-Pair
aldo_export
inline void aldo_booltolnp(bool v, bool *aldo_noalias hi,
                           bool *aldo_noalias lo) aldo_nothrow
{
    *hi = v;
    *lo = !v;
}

aldo_export
const char *aldo_ntmirror_name(enum aldo_ntmirror m) aldo_nothrow;
#include "bridgeclose.h"

#endif

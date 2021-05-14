//
//  memorymap.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/9/21.
//

#ifndef Aldo_emu_memorymap_h
#define Aldo_emu_memorymap_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A Memory Map is a mapping over an N-bit address space, where N -> (0, 16];
// this memory space is then divided into pages of width P bits where P <= N
// and N % P = 0; finally the memory map is additionally divided into memory
// banks which control read/write access of some contiguous subset of the
// N-bit address space and wire it to an actual underlying memory space;

// Memory Map - how much address space is available
// Memory Page - the granularity of the address space
// (i.e. how flexible is the memory layout)
// Memory Bank - how a range of addresses behave and bind to an underlying
// bus component
// (e.g. RAM, ROM, bank-switched, memory-mapped registers, I/O, etc)

// a Memory Bank encompasses one or more pages and banks cannot overlap in
// either page space or address space; the activation of a bank that overlaps
// existing banks will deactivate the existing banks; this implies (and allows)
// for gaps in a memory map, it is a valid configuration to have some address
// ranges not wire to anything;

// for example a 16KB read-only bank from $8000 - $BFFF over 64KB of memory,
// where all 64KB are made available through bank-switching;
// or an 8KB read/write address bank $0000 - $1FFF over 2KB of memory,
// mirroring a 2KB RAM across the 8KB address window;

// reads and writes may be mediated by a read- or write-policy unique to
// a bank, emulating underlying circuitry that may be present at the given bus
// locations (e.g. iNES mappers).

typedef struct memorymap memmap;

typedef bool rpolicy(memmap *, uint8_t, uint16_t, uint8_t *restrict,
                     void *ctx);
typedef bool wpolicy(memmap *, uint8_t *restrict, uint16_t, uint8_t,
                     void *ctx);

enum memorymode {
    MMODE_NONE,
    MMODE_READ,
    MMODE_WRITE,
};

memmap *memmap_new(size_t addrwidth, size_t pagewidth);
void memmap_free(memmap *self);

size_t memmap_size(memmap *self);
size_t memmap_capacity(memmap *self);

// TODO: should size be in bitwidth as well
bool memmap_addbank(memmap *self, size_t size, uint8_t mem[size],
                    uint16_t addrmin, uint16_t addrmax, enum memorymode mode,
                    rpolicy rp, wpolicy wp, void *policy_ctx);

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d);
bool memmap_write(memmap *self, uint16_t addr, uint8_t d);

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr);

#endif

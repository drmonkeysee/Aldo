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

// A Memory Map is an N-bit address space, where N -> (0, 16] representing
// the available address space of a processing unit; the memory map is divided
// into bus devices which manage read/write access of some contiguous subset
// of the N-bit address space and wire it to underlying functionality such as
// standard memory, switchable RAM/ROM banks, memory-mapped registers,
// I/O controllers, etc;

// the partitioning of address space to bus devices is fixed at creation time,
// however different bus device implementations can be swapped in and out of
// their respective locations (e.g. swapping a NES cartridge, or a disk drive);
// it is also possible for no device to be mapped to a particular address
// partition, in which case there is no hardware wired to that bus location;

// some examples: a 64KB ROM board wired to $8000 - $BFFF, exposing 16KBs at a
// time via bank-switching circuitry on the ROM board;
// or a 2KB RAM chip wired to an 8KB address range $0000 - $1FFF,
// mirroring the memory 4x at that bus location.
typedef struct memorymap memmap;
typedef bool rpolicy(void *ctx, uint16_t, uint8_t *restrict);
typedef bool wpolicy(void *ctx, uint16_t, uint8_t);

// NOTE: a bus device must be copyable and thus cannot own its pointer members;
// make sure something else manages the lifetime of ctx.
struct busdevice {
    rpolicy *read;  // Read policy (if NULL device is write-only)
    wpolicy *write; // Write policy (if NULL device is read-only)
    void *ctx;      // Policy context
};

memmap *memmap_new(size_t addrwidth);
void memmap_free(memmap *self);

uint16_t memmap_maxaddr(memmap *self);

bool memmap_add(memmap *self, struct busdevice d);

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d);
bool memmap_write(memmap *self, uint16_t addr, uint8_t d);

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr);

#endif

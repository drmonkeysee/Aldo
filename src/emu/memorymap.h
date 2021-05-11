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

// A Memory Map is a vector of Memory Banks covering 16-bit address spaces;
// a memory map may be non-contiguous (there may be gaps in the full 16-bit
// space) but guaranteed non-overlapping (one address always maps to a single
// bank).

// A Memory Bank is a 16-bit address range on to some underlying memory space;
// e.g. an 8KB bank from $B000 - $CFFF on a 64KB PRG ROM; or an 8KB
// address space $0000 - $1FFF with mask 0x7ff, mirroring a 2KB RAM across the
// 8KB address window; reads and writes are mediated by a read- and
// write-policy unique to a bank, emulating underlying circuitry that may be
// present at the given bus locations (e.g. iNES mappers).

typedef struct memorymap memmap;
typedef struct memorybank membank;

typedef bool rpolicy(membank *, uint16_t, uint8_t *restrict);
typedef bool wpolicy(membank *, uint16_t, uint8_t, memmap *);

// NOTE: memorybank does not own any of its pointer members and is safe to copy
struct memorybank {
    size_t size;        // Bank Size
    uint8_t *mem;       // Bank Memory
    rpolicy *read;      // Memory-read policy; if not set the bank acts as a
                        // readable array of bytes.
    wpolicy *write;     // Memory-write policy; if not set the bank acts as a
                        // writable array of bytes.
    void *pctx;         // Context object for RW policies
    uint16_t addrmin,   // Min Address covered by this bank
             addrmax,   // Max Address covered by this bank
             addrmask;  // Address mask for partially-decoded bank addresses
};

memmap *memmap_new(void);
void memmap_free(memmap *self);

size_t memmap_size(memmap *self);
size_t memmap_capacity(memmap *self);

bool memmap_add(memmap *self, const membank *b);

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d);
bool memmap_write(memmap *self, uint16_t addr, uint8_t d);

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr);

#endif

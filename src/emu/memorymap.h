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

// A Memory Bank is a 16-bit address window on to some underlying memory space;
// e.g. an 8KB bank from $B000 - $CFFF on an underlying 64KB PRG ROM space;
// or a 2KB bank from $0000 - $7FFF with mask 0x800, mirroring $0000 - $07FF
// through the full 2KB bank.

// NOTE: memorybank does not own any of its pointer members and is safe to copy
struct memorybank {
    size_t size;        // Bank Size
    uint8_t *mem;       // Bank Memory
    void *mapper;       // Mapper-owned Bank
    uint16_t addrmin,   // Min Address covered by this bank
             addrmax,   // Max Address covered by this bank
             addrmask;  // Address mask applied to incoming addresses
};

typedef struct memorymap memmap;

memmap *memmap_new(void);
void memmap_free(memmap *self);

#define memmap_add(s, b) memmap_addnbanks(s, 1, b)
#define memmap_addbanks(s, bs) \
memmap_addnbanks(s, sizeof (bs) / sizeof (bs)[0], bs)
bool memmap_addnbanks(memmap *self, size_t count,
                      const struct memorybank banks[count]);

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr);

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d);
bool memmap_write(memmap *self, uint16_t addr, uint8_t d);

#endif

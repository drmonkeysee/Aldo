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

// X(symbol, value, error string)
#define MEMMAP_ERRCODE_X \
X(MMAP_PARTITION_RANGE, -1, "PARTITION OUT OF RANGE")

enum {
#define X(s, v, e) s = v,
    MEMMAP_ERRCODE_X
#undef X
};

// A Memory Map is an N-bit structure, where N -> (0, 16], representing
// the available address space of a processing unit; the memory map is divided
// into a fixed-set of address-range partitions where each partition is
// optionally linked to some functional component such as memory, switchable
// RAM/ROM banks, memory-mapped registers, I/O controllers, etc;

// although the partitions are fixed, components can be swapped in and out
// of these partitions provided the component maps to the partition's address
// range; it is also possible for a partition to have no linked
// component, in which case the address range is not mapped to anything;

// some examples: a 64KB ROM board wired to $8000 - $BFFF, exposing 16KBs at a
// time via bank-switching circuitry on the ROM board;
// or a 2KB RAM chip wired to an 8KB address range $0000 - $1FFF,
// mirroring the memory 4x at that bus location.
typedef struct memorymap memmap;
typedef bool rpolicy(void *ctx, uint16_t, uint8_t *restrict);
typedef bool wpolicy(void *ctx, uint16_t, uint8_t);

// A Memory Link is a connection from a processing unit's memory map to
// a component responsible for handling reads and writes to the connected
// address partition; memlinks are passed by value and do not own their
// pointer members.
struct memlink {
    rpolicy *read;  // Read policy (if NULL, component is write-only)
    wpolicy *write; // Write policy (if NULL, component is read-only)
    void *ctx;      // Policy context
};

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *memmap_errstr(int err);

// NOTE: n is partition count, while variadic arguments specify the address at
// which each partition starts *excluding 0 which is always implied*; thus
// there is one less variadic argument than the value of n;
// e.g. (16, 4, 0x2000, 0x4000, 0x8000) ->
// - 16-bit address space
// - 4 partitions
// - mapped as [$0000 - $1FFF, $2000 - $3FFF, $4000 - $7FFF, $8000 - $FFFF]
memmap *memmap_new(size_t addrwidth, size_t n, ...);
void memmap_free(memmap *self);

size_t memmap_count(memmap *self);
uint16_t memmap_maxaddr(memmap *self);
// NOTE: get start address of partition i, returns < 0 if i is out of range
int memmap_pstart(memmap *self, size_t i);

// NOTE: value of addr can be anywhere in the range of the targeted partition
// NOTE: if prev is not null it is set to the contents of the old link
bool memmap_swap(memmap *self, uint16_t addr, struct memlink ml,
                 struct memlink *prev);
inline bool memmap_set(memmap *self, uint16_t addr, struct memlink ml)
{
    return memmap_swap(self, addr, ml, NULL);
}

bool memmap_read(memmap *self, uint16_t addr, uint8_t *restrict d);
bool memmap_write(memmap *self, uint16_t addr, uint8_t d);

bool memmap_copy(memmap *self, size_t size, uint8_t buf[restrict size],
                 uint16_t minaddr, uint16_t maxaddr);

#endif

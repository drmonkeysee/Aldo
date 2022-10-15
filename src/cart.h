//
//  cart.h
//  Aldo
//
//  Created by Brandon Stansbury on 5/1/21.
//

#ifndef Aldo_cart_h
#define Aldo_cart_h

#include "bus.h"
#include "cartinfo.h"
#include "snapshot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// X(symbol, value, error string)
#define CART_ERRCODE_X \
X(CART_ERR_UNKNOWN, -1, "UNKNOWN CART LOAD ERROR") \
X(CART_ERR_IO, -2, "FILE READ ERROR") \
X(CART_ERR_IMG_SIZE, -3, "ROM IMAGE TOO LARGE") \
X(CART_ERR_EOF, -4, "UNEXPECTED EOF") \
X(CART_ERR_OBSOLETE, -5, "OBSOLETE FORMAT") \
X(CART_ERR_ADDR_UNAVAILABLE, -6, "BUS ADDRESS UNAVAILABLE") \
X(CART_ERR_FORMAT, -7, "FORMAT UNSUPPORTED") \
X(CART_ERR_FMT, -8, "FORMATTED OUTPUT FAILURE") \
X(CART_ERR_ERNO, -9, "SYSTEM ERROR")

enum {
#define X(s, v, e) s = v,
    CART_ERRCODE_X
#undef X
};

enum {
    CART_FMT_SIZE = 17, // Format extended name is at most 16 chars
};

struct blockview {
    size_t ord, size;
    const uint8_t *mem; // Non-owning Pointer
};

typedef struct cartridge cart;

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *cart_errstr(int err);
const char *cart_formatname(enum cartformat format);
const char *cart_mirrorname(enum nt_mirroring mirror);

// NOTE: if returns non-zero error code, *c is unmodified
int cart_create(cart **c, const char *restrict filepath);
void cart_free(cart *self);

void cart_getinfo(cart *self, struct cartinfo *info);
struct blockview cart_prgblock(cart *self, size_t i);
struct blockview cart_chrblock(cart *self, size_t i);

int cart_cpu_connect(cart *self, bus *b, uint16_t addr);
void cart_cpu_disconnect(cart *self, bus *b, uint16_t addr);

void cart_write_info(cart *self, FILE *f, bool verbose);
void cart_write_dis_header(cart *self, FILE *f);

// NOTE: returns a pointer to a statically allocated string;
// **WARNING**: do not write through or free this pointer!
const char *cart_filename(const struct cartinfo *info);
int cart_format_extendedname(const struct cartinfo *info,
                             char buf[restrict static CART_FMT_SIZE]);
void cart_snapshot(cart *self, struct console_state *snapshot);

#endif

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
X(CART_ERR_FMT, -8, "FORMATTED OUTPUT FAILURE")

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

#include "bridgeopen.h"
//
// Export
//

br_libexport
const char *cart_errstr(int err) br_nothrow;
br_libexport br_checkerror
int cart_format_extname(const struct cartinfo *info,
                        char buf[br_noalias_csz(CART_FMT_SIZE)]) br_nothrow;

// NOTE: if returns non-zero error code, *c is unmodified
br_libexport br_checkerror
int cart_create(cart **c, FILE *f) br_nothrow;
br_libexport
void cart_free(cart *self) br_nothrow;

br_libexport
void cart_write_info(cart *self, const char *br_noalias name, bool verbose,
                     FILE *f) br_nothrow;

//
// Internal
//

const char *cart_formatname(enum cartformat format) br_nothrow;
const char *cart_mirrorname(enum nt_mirroring mirror) br_nothrow;

void cart_getinfo(cart *self, struct cartinfo *info) br_nothrow;
struct blockview cart_prgblock(cart *self, size_t i) br_nothrow;
struct blockview cart_chrblock(cart *self, size_t i) br_nothrow;

int cart_cpu_connect(cart *self, bus *b, uint16_t addr) br_nothrow;
void cart_cpu_disconnect(cart *self, bus *b, uint16_t addr) br_nothrow;

void cart_write_dis_header(cart *self, const char *br_noalias name,
                           FILE *f) br_nothrow;

void cart_snapshot(cart *self, struct console_state *snapshot) br_nothrow;
#include "bridgeclose.h"

#endif

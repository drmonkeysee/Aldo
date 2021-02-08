//
//  decode.c
//  Aldo
//
//  Created by Brandon Stansbury on 2/3/21.
//

#include "decode.h"

#define enc(in, cs) {in, cs}

// TODO: fill this out
const struct decoded Decode[256];
const size_t DecodeSize = sizeof Decode / sizeof Decode[0];

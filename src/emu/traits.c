//
//  traits.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/21/21.
//

#include "traits.h"

#define CART_MIN 0x4020u
#define CART_MAX 0xffffu

const uint16_t CpuRamMaxAddr = 0x1fff,
               CpuRamAddrMask = RAM_SIZE - 1,
               CpuCartMinAddr = CART_MIN,
               CpuCartMaxAddr = CART_MAX,
               CpuCartAddrMask = CART_MAX - CART_MIN,

               NmiVector = 0xfffa,
               ResetVector = 0xfffc,
               IrqVector = 0xfffe;

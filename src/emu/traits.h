//
//  traits.h
//  Aldo
//
//  Created by Brandon Stansbury on 1/15/21.
//

#ifndef Aldo_emu_traits_h
#define Aldo_emu_traits_h

// CPU Memory Map
#define RAM_SIZE 0x800u
#define CART_MIN_CPU_ADDR 0x4020u
#define CART_MAX_CPU_ADDR 0xffffu
#define CART_CPU_ADDR_MASK 0xbfdfu  // 0xffff - 0x4020u

// Interrupt Vectors
#define NMI_VECTOR 0xfffau
#define RESET_VECTOR 0xfffcu
#define IRQ_VECTOR 0xfffeu

#endif

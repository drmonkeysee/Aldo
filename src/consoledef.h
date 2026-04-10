//
//  consoledef.h
//  Aldo
//
//  Created by Brandon Stansbury on 4/10/26.
//

#ifndef Aldo_consoledef_h
#define Aldo_consoledef_h

#include "console.h"

// Console Base Definition, providing a common interface and data type for
// emulation of all supported Aldo consoles.
struct aldo_console_base {
    enum aldo_console_type type;
};

#endif

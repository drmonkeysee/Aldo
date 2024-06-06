//
//  uisdl.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifndef Aldo_gui_uisdl_hpp
#define Aldo_gui_uisdl_hpp

#include "debug.h"
#include "nes.h"

struct gui_platform;

#ifdef __cplusplus
namespace aldo
{
#endif

#include "bridgeopen.h"
br_checkerror
int ui_sdl_runloop(const struct gui_platform* platform, debugger* debug,
                   nes* console) br_nothrow;
#include "bridgeclose.h"

#ifdef __cplusplus
}
#endif

#endif

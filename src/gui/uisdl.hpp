//
//  uisdl.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifndef Aldo_gui_uisdl_hpp
#define Aldo_gui_uisdl_hpp

#include "guiplatform.h"

#ifdef __cplusplus
namespace aldo
{
#endif

#include "bridgeopen.h"
br_checkerror
int ui_sdl_init(const struct gui_platform* platform) br_nothrow;
int ui_sdl_runloop(br_noargs) br_nothrow;
#include "bridgeclose.h"

#ifdef __cplusplus
}
#endif

#endif

//
//  uisdl.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifndef Aldo_gui_uisdl_hpp
#define Aldo_gui_uisdl_hpp

#include "cliargs.h"
#include "guiplatform.h"
#include "ui.h"

#ifdef __cplusplus
namespace aldo
{
#endif
#include "bridgeopen.h"

aldo_checkreturn("error code")
int ui_sdl_init(const struct cliargs* args,
                const struct gui_platform* platform,
                ui_loop** loop) aldo_nothrow;

#include "bridgeclose.h"
#ifdef __cplusplus
}
#endif

#endif

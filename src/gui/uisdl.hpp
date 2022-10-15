//
//  uisdl.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifndef Aldo_gui_uisdl_hpp
#define Aldo_gui_uisdl_hpp

#include "control.h"
#include "guiplatform.h"
#include "ui.h"

#ifdef __cplusplus
namespace aldo
{
#endif
#include "interopopen.h"

aldo_nodiscard("error code")
int ui_sdl_init(struct ui_interface* ui,
                const struct control* appstate,
                const struct gui_platform* platform) aldo_noexcept;

#include "interopclose.h"
#ifdef __cplusplus
}
#endif

#endif

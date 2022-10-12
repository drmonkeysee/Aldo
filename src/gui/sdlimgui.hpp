//
//  sdlimgui.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/11/22.
//

#ifndef Aldo_gui_sdlimgui_hpp
#define Aldo_gui_sdlimgui_hpp

#include "ui.h"

#ifdef __cplusplus
namespace aldo
{
extern "C"
{
#define ALDO_NOEXCEPT noexcept
#else
#define ALDO_NOEXCEPT
#endif

int sdlimgui_init(struct ui_interface *ui) ALDO_NOEXCEPT;

#ifdef __cplusplus
}
}
#endif
#undef ALDO_NOEXCEPT

#endif

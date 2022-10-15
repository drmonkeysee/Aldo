//
//  uisdl.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifndef Aldo_gui_uisdl_hpp
#define Aldo_gui_uisdl_hpp

#include "ui.h"

#ifdef __cplusplus
namespace aldo
{
#endif
#include "interopopen.h"

ALDO_NODISCARD("error code")
int ui_sdl_init(struct ui_interface* ui) ALDO_NOEXCEPT;

#include "interopclose.h"
#ifdef __cplusplus
}
#endif

#endif

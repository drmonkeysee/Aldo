//
//  sdlimgui.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/11/22.
//

#ifndef Aldo_gui_sdlimgui_hpp
#define Aldo_gui_sdlimgui_hpp

#ifdef __cplusplus
namespace aldo
{
extern "C"
{
#define ALDO_NOARG
#define ALDO_NOEXCEPT noexcept
#else
#define ALDO_NOARG void
#define ALDO_NOEXCEPT
#endif

int sdlimgui_init(ALDO_NOARG) ALDO_NOEXCEPT;

#ifdef __cplusplus
}
}
#endif
#undef ALDO_NOEXCEPT
#undef ALDO_NOARG

#endif

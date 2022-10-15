//
//  uisdl.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifndef Aldo_gui_uisdl_hpp
#define Aldo_gui_uisdl_hpp

struct ui_interface;

#ifdef __cplusplus
namespace aldo
{
extern "C"
{
#endif

int ui_sdl_init(struct ui_interface* ui);

#ifdef __cplusplus
}
}
#endif

#endif

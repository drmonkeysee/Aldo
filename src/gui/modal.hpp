//
//  modal.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/24/23.
//

#ifndef Aldo_gui_modal_hpp
#define Aldo_gui_modal_hpp

struct gui_platform;

namespace aldo
{

class Emulator;

namespace modal
{

bool loadROM(Emulator& emu, const gui_platform& p);
bool loadBreakpoints(Emulator& emu, const gui_platform& p);
bool exportBreakpoints(Emulator& emu, const gui_platform& p);
bool loadPalette(Emulator& emu, const gui_platform& p);

}

}

#endif

//
//  modal.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/24/23.
//

#ifndef Aldo_gui_modal_hpp
#define Aldo_gui_modal_hpp

namespace aldo
{

class Emulator;
class MediaRuntime;

namespace modal
{

bool loadROM(Emulator& emu, const MediaRuntime& mr);
bool loadBreakpoints(Emulator& emu, const MediaRuntime& mr);
bool exportBreakpoints(Emulator& emu, const MediaRuntime& mr);
bool loadPalette(Emulator& emu, const MediaRuntime& mr);

}

}

#endif

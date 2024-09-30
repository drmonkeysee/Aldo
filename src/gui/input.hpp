//
//  input.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#ifndef Aldo_gui_input_hpp
#define Aldo_gui_input_hpp

namespace aldo
{

class Emulator;
class MediaRuntime;
struct viewstate;

namespace input
{

void handle(Emulator& emu, viewstate& vs, const MediaRuntime& mr);

}

}

#endif

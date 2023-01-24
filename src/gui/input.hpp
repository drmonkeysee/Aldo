//
//  input.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#ifndef Aldo_gui_input_hpp
#define Aldo_gui_input_hpp

struct guiplatform;

namespace aldo
{

class Emulator;
struct viewstate;

namespace input
{

void handle(Emulator& emu, viewstate& vs, const guiplatform& p);

}

}

#endif

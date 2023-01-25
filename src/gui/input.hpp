//
//  input.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#ifndef Aldo_gui_input_hpp
#define Aldo_gui_input_hpp

struct gui_platform;

namespace aldo
{

class Emulator;
struct viewstate;

namespace input
{

void handle(Emulator& emu, viewstate& vs, const gui_platform& p);

}

}

#endif

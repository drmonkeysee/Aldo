//
//  input.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/28/22.
//

#ifndef Aldo_gui_input_hpp
#define Aldo_gui_input_hpp

#include "nes.h"

namespace aldo
{

struct viewstate;

void handle_input(viewstate& s, nes* console);

}

#endif

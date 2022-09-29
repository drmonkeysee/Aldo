//
//  gui.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#ifndef Aldo_gui_gui_hpp
#define Aldo_gui_gui_hpp

#include "options.h"

namespace aldo
{

int rungui_with_args(int argc, char* argv[],
                     const aldo::aldo_guiopts& options);

}

#endif

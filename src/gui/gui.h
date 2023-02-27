//
//  gui.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#ifndef Aldo_gui_gui_h
#define Aldo_gui_gui_h

#include "guiplatform.h"

#include "bridgeopen.h"
br_checkerror
int gui_run(const struct gui_platform *platform) br_nothrow;
#include "bridgeclose.h"

#endif

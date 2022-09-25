//
//  options.h
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#ifndef Aldo_gui_options_h
#define Aldo_gui_options_h

#ifdef __cplusplus
namespace aldo
{
#else
#include <stdbool.h>
#endif

struct initopts {
    bool hi_dpi;
    float render_scale_factor;
};

#ifdef __cplusplus
}
#endif

#endif

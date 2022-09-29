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

struct aldo_guiopts {
    float render_scale_factor;
    bool hi_dpi;
};

#ifdef __cplusplus
}
#endif

#endif

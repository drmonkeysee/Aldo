//
//  options.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#ifndef Aldo_gui_options_hpp
#define Aldo_gui_options_hpp

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
using guiopts = aldo_guiopts;

}
#endif

#endif

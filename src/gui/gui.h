//
//  gui.h
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#ifndef Aldo_gui_gui_h
#define Aldo_gui_gui_h

#include "options.h"

#ifdef __cplusplus
#define ALDO_EXPORT extern "C"
#define ALDO_ARGV char* argv[]
#define ALDO_OPTS const aldo::aldo_guiopts* options
#define ALDO_NOEXCEPT noexcept
namespace aldo
{
#else
#define ALDO_EXPORT
#define ALDO_ARGV char *argv[argc+1]
#define ALDO_OPTS const struct aldo_guiopts *options
#define ALDO_NOEXCEPT
#endif

ALDO_EXPORT int aldo_rungui_with_args(int argc, ALDO_ARGV,
                                      ALDO_OPTS) ALDO_NOEXCEPT;

#ifdef __cplusplus
}
#endif
#undef ALDO_NOEXCEPT
#undef ALDO_OPTS
#undef ALDO_ARGV
#undef ALDO_EXPORT

#endif
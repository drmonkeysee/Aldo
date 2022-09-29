//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#import <AppKit/AppKit.h>

#include "gui.h"
#include "options.h"

// NOTE: include SDL for potential SDL_main replacement
#include <SDL2/SDL.h>

#include <stdbool.h>

int main(int argc, char *argv[argc+1])
{
    SDL_Log("Aldo GUI started...");
    struct aldo_guiopts opts = {.hi_dpi = true};
    @autoreleasepool {
        opts.render_scale_factor = NSScreen.mainScreen.backingScaleFactor;
    }
    return aldo_rungui_with_args(argc, argv, &opts);
}

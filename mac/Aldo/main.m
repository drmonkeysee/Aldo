//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#import <AppKit/AppKit.h>

#include "gui.hpp"
#include "options.hpp"

// NOTE: include SDL for potential SDL_main replacement
#include <SDL2/SDL.h>

int main(int argc, char *argv[argc+1])
{
    SDL_Log("Aldo GUI started...");
    struct aldo_guiopts opts;
    @autoreleasepool {
        opts.hi_dpi = [[NSBundle.mainBundle
                       objectForInfoDictionaryKey:@"NSHighResolutionCapable"]
                       boolValue];
        opts.render_scale_factor = NSScreen.mainScreen.backingScaleFactor;
    }
    return aldo_rungui_with_args(argc, argv, &opts);
}

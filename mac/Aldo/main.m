//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#include "gui.h"
#include "guiplatform.h"

// NOTE: always include SDL with main for potential SDL_main replacement
#include <SDL2/SDL.h>

#include <stdlib.h>

int main(int argc, char *argv[argc+1])
{
    SDL_Log("Aldo GUI started...");

    struct gui_platform platform;
    if (!gui_platform_init(&platform)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create platform interface!");
        return EXIT_FAILURE;
    }

    return gui_run(argc, argv, &platform);
    /*struct aldo_guiopts opts;
    @autoreleasepool {
        opts.hi_dpi = [[NSBundle.mainBundle
                       objectForInfoDictionaryKey:@"NSHighResolutionCapable"]
                       boolValue];
        opts.render_scale_factor = NSScreen.mainScreen.backingScaleFactor;
    }
    return aldo_rungui_with_args(argc, argv, &opts);*/
}

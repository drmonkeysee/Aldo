//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#import "Aldo-Swift.h"
#import <AppKit/AppKit.h>

#include "gui.h"
#include "guiplatform.h"

// NOTE: always include SDL with main in case SDL_main applies; this is also
// why the app entrypoint is in Objective-C rather than Swift.
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <stdlib.h>

// NOTE: this function uses preprocessor macros and can't bridge to Swift
static float render_scale_factor(void *sdl_win)
{
    SDL_SysWMinfo winfo;
    SDL_VERSION(&winfo.version);
    if (SDL_GetWindowWMInfo(sdl_win, &winfo)) {
        if (winfo.subsystem == SDL_SYSWM_COCOA) {
            NSWindow *const native = winfo.info.cocoa.window;
            return (float)native.backingScaleFactor;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Unexpected window subsystem found: %d",
                         winfo.subsystem);
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to retrieve window info: %s", SDL_GetError());
    }
    return 1.0f;
}

static int run_app(void)
{
    struct gui_platform platform;
    if (![MacPlatform setup:&platform withScaleFunc:render_scale_factor]) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create platform interface!");
        return EXIT_FAILURE;
    }
    const int result = gui_run(&platform);
    platform.cleanup(&platform.ctx);
    return result;
}

//
// App Entrypoint
//

int main(int argc, char *argv[argc+1])
{
    (void)argv;
    SDL_Log("Aldo GUI started...");

    @autoreleasepool {
        return run_app();
    }
}

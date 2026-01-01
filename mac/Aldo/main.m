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

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdlib.h>

static float render_scale_factor(void *sdlwindow)
{
    auto cocoa_win = (__bridge NSWindow *)SDL_GetPointerProperty(
         SDL_GetWindowProperties(sdlwindow),
         SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
         nullptr
    );
    if (cocoa_win) {
        return (float)cocoa_win.backingScaleFactor;
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to retrieve window info: %s", SDL_GetError());
        return 1.0f;
    }
}

static int run_app()
{
    struct gui_platform platform;
    if (![MacPlatform setup:&platform withScaleFunc:render_scale_factor]) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Failed to create platform interface!");
        return EXIT_FAILURE;
    }
    return gui_run(&platform);
}

//
// MARK: - App Entrypoint
//

int main(int, char *[])
{
    SDL_Log("Aldo GUI started...");

    @autoreleasepool {
        return run_app();
    }
}

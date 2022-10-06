//
//  macplatform.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 10/5/22.
//

#import <AppKit/AppKit.h>

#include "guiplatform.h"

#include <SDL2/SDL_syswm.h>

static bool is_hidpi(void)
{
    return [[NSBundle.mainBundle
             objectForInfoDictionaryKey:@"NSHighResolutionCapable"] boolValue];
}

static float render_scale_factor(SDL_Window *win)
{
    SDL_SysWMinfo winfo;
    SDL_VERSION(&winfo.version);
    if (SDL_GetWindowWMInfo(win, &winfo)) {
        if (winfo.subsystem == SDL_SYSWM_COCOA) {
            NSWindow *const native = winfo.info.cocoa.window;
            return native.backingScaleFactor;
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Unexpected window subsystem found: %d",
                         winfo.subsystem);
        }
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unable to retrieve window info: %s", SDL_GetError());
    }
    return 1.0;
}

//
// Public Interface
//

bool gui_platform_init(struct gui_platform *platform)
{
    *platform = (struct gui_platform){is_hidpi, render_scale_factor};
    return true;
}

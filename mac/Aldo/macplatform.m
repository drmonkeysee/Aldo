//
//  macplatform.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 10/5/22.
//

#import <AppKit/AppKit.h>

#include "guiplatform.h"

#include <SDL2/SDL_syswm.h>

#include <stddef.h>
#include <stdlib.h>

static char *restrict AppName;

static const char *appname(void)
{
    @autoreleasepool {
        static const NSStringEncoding encoding = NSUTF8StringEncoding;
        NSString *const displayName = [NSBundle.mainBundle
                                       objectForInfoDictionaryKey:
                                           @"CFBundleDisplayName"];
        const NSUInteger len = [displayName
                                lengthOfBytesUsingEncoding: encoding];
        if (len > 0) {
            const NSUInteger sz = len + 1;
            char *const buf = malloc(sz);
            if ([displayName getCString:buf maxLength:sz encoding:encoding]) {
                AppName = buf;
            } else {
                free(buf);
            }
        }
    }
    return AppName;
}

static bool is_hidpi(void)
{
    @autoreleasepool {
        return [[NSBundle.mainBundle
                 objectForInfoDictionaryKey:@"NSHighResolutionCapable"]
                boolValue];
    }
}

static float render_scale_factor(SDL_Window *win)
{
    SDL_SysWMinfo winfo;
    SDL_VERSION(&winfo.version);
    if (SDL_GetWindowWMInfo(win, &winfo)) {
        if (winfo.subsystem == SDL_SYSWM_COCOA) {
            @autoreleasepool {
                NSWindow *const native = winfo.info.cocoa.window;
                return native.backingScaleFactor;
            }
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

static void cleanup(void)
{
    free(AppName);
    AppName = NULL;
}

//
// Public Interface
//

bool gui_platform_init(struct gui_platform *platform)
{
    *platform = (struct gui_platform){
        appname,
        is_hidpi,
        render_scale_factor,
        cleanup,
    };
    return true;
}

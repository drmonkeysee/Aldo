//
//  macplatform.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 10/5/22.
//

#import <AppKit/AppKit.h>

#include "guiplatform.h"

#include <SDL2/SDL_syswm.h>

#include <assert.h>

static bool fillbuf_from_string(size_t sz, char buf[sz], size_t *len,
                                NSString *string)
{
    static const NSStringEncoding encoding = NSUTF8StringEncoding;
    const NSUInteger strLength = [string lengthOfBytesUsingEncoding: encoding];
    if (len) {
        *len = strLength;
    }
    if (buf && 0 < strLength && strLength < sz) {
        return [string getCString:buf maxLength:sz encoding:encoding];
    }
    return false;
}

static bool appname(size_t sz, char buf[sz], size_t *len)
{
    @autoreleasepool {
        NSString *const displayName = [NSBundle.mainBundle
                                       objectForInfoDictionaryKey:
                                           @"CFBundleDisplayName"];
        return fillbuf_from_string(sz, buf, len, displayName);
    }
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
    assert(win != NULL);

    SDL_SysWMinfo winfo;
    SDL_VERSION(&winfo.version);
    if (SDL_GetWindowWMInfo(win, &winfo)) {
        if (winfo.subsystem == SDL_SYSWM_COCOA) {
            @autoreleasepool {
                NSWindow *const native = winfo.info.cocoa.window;
                return (float)native.backingScaleFactor;
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
    return 1.0f;
}

static bool open_file(size_t sz, char buf[sz], size_t *len)
{
    @autoreleasepool {
        NSOpenPanel *const panel = [NSOpenPanel openPanel];
        panel.message = @"Choose a ROM file";
        const NSModalResponse r = [panel runModal];
        if (r == NSModalResponseOK) {
            return fillbuf_from_string(sz, buf, len, panel.URL.absoluteString);
        }
        if (len) {
            *len = 0;
        }
        return false;
    }
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
        open_file,
    };
    return true;
}

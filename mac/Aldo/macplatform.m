//
//  macplatform.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 10/5/22.
//

#import <AppKit/AppKit.h>

#include "guiplatform.h"

#include <SDL2/SDL_syswm.h>

#include <string.h>

static bool appname(size_t sz, char buf[restrict sz], size_t *len)
{
    @autoreleasepool {
        static const NSStringEncoding encoding = NSUTF8StringEncoding;
        NSString *const displayName = [NSBundle.mainBundle
                                       objectForInfoDictionaryKey:
                                           @"CFBundleDisplayName"];
        const NSUInteger strLen = [displayName
                                   lengthOfBytesUsingEncoding: encoding];
        if (len) {
            *len = strLen;
        }
        if (buf && 0 < strLen && strLen < sz) {
            return [displayName getCString:buf maxLength:sz encoding:encoding];
        }
        return false;
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

static bool open_file(size_t sz, char buf[restrict sz], size_t *len)
{
    @autoreleasepool {
        NSOpenPanel *const panel = [NSOpenPanel openPanel];
        panel.message = @"Choose a ROM file";
        const NSModalResponse r = [panel runModal];
        if (r == NSModalResponseOK) {
            NSURL *const path = panel.URL;
            const size_t strLen = strlen(path.fileSystemRepresentation);
            if (len) {
                *len = strLen;
            }
            if (buf && 0 < strLen && strLen < sz) {
                return [path getFileSystemRepresentation:buf maxLength:sz];
            }
        } else if (len) {
            *len = 0;
        }
        return false;
    }
}

static bool display_error(const char *title, const char *message)
{
    @autoreleasepool {
        NSAlert *const modal = [[NSAlert alloc] init];
        modal.messageText = [NSString stringWithUTF8String:title];
        modal.informativeText = [NSString stringWithUTF8String:message];
        modal.alertStyle = NSAlertStyleWarning;
        return [modal runModal] == NSModalResponseOK;
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
        display_error,
    };
    return true;
}

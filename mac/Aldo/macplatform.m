//
//  macplatform.m
//  Aldo
//
//  Created by Brandon Stansbury on 10/5/22.
//

#import <Foundation/Foundation.h>

#include "guiplatform.h"

static bool is_hidpi(void)
{
    return [[NSBundle.mainBundle
             objectForInfoDictionaryKey:@"NSHighResolutionCapable"] boolValue];
}

//
// Public Interface
//

bool gui_platform_init(struct gui_platform *platform)
{
    *platform = (struct gui_platform){is_hidpi};
    return true;
}

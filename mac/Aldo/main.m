//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#include "gui.h"
#include "guiplatform.h"

// NOTE: always include SDL with main in case SDL_main applies
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
}

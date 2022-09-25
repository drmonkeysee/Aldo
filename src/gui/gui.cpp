//
//  gui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#include "gui.h"
#include "gui.hpp"

#include <SDL2/SDL.h>

#include <exception>

#include <cstdlib>

//
// Public Interface
//

int aldo::run_with_args(int, char*[], const initopts&)
{
    return EXIT_SUCCESS;
}

int aldo::aldo_run_with_args(int argc, char* argv[],
                             const initopts *opts) noexcept
{
    try {
        return run_with_args(argc, argv, *opts);
    } catch (const std::exception& ex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unhandled error in Aldo: %s", ex.what());
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown error in Aldo!");
    }
    return EXIT_FAILURE;
}

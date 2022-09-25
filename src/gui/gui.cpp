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

namespace
{

int sdl_demo(const aldo::initopts&)
{
    return EXIT_SUCCESS;
}

}

//
// Public Interface
//

int aldo::run_with_args(int, char*[], const aldo::initopts& opts)
{
    return sdl_demo(opts);
}

int aldo::aldo_run_with_args(int argc, char* argv[],
                             const aldo::initopts *opts) noexcept
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

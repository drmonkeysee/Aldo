//
//  main.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 2/26/23.
//

#include "gui.h"
#include "guiplatform.h"

#include <iostream>
#include <cstdlib>

namespace
{

[[maybe_unused]]
auto run_app() noexcept
{
    // TODO: fill this out with Linux-specific platform functions
    gui_platform platform{};
    return gui_run(&platform);
}

}

int main(int, char*[])
{
    std::cout << "Linux build not yet implemented\n";
    return EXIT_SUCCESS;
}

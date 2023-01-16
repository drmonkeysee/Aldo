//
//  emutypes.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/16/23.
//

#include "emutypes.hpp"

#include <concepts>
#include <fstream>
#include <string>

static_assert(std::same_as<std::ifstream::char_type, aldo::et::tchar>,
              "Text stream type does not match emulator text type");

static_assert(std::same_as<std::string::value_type, aldo::et::tchar>,
              "Text string type does not match emulator text type");

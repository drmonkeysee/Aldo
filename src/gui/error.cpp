//
//  error.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/6/22.
//

#include "error.hpp"

#include <SDL2/SDL.h>

#include <sstream>
#include <cstring>

namespace
{

auto build_sdl_error(std::string message)
{
    message += " (";
    message += SDL_GetError();
    message += ')';
    return message;
}

auto build_display_what(std::string_view title, std::string_view message)
{
    std::string what{title};
    what += ": ";
    what += message;
    return what;
}

auto errno_message(std::string_view label, int err)
{
    std::stringstream s;
    s << label << " [" << strerror(err) << " (" << err << ")]";
    return s.str();
}

}

//
// Public Interface
//

aldo::SdlError::SdlError(std::string message)
: std::runtime_error{build_sdl_error(std::move(message))} {}

aldo::DisplayError::DisplayError(std::string title, std::string message)
: std::runtime_error{std::move(title)}, msg{std::move(message)},
    wht{build_display_what(this->title(), this->message())} {}

aldo::DisplayError::DisplayError(std::string title, std::string_view label,
                                 int errnoVal)
: DisplayError{std::move(title), errno_message(label, errnoVal)} {}

//
// Private Interface
//

std::string aldo::DisplayError::emuErrMessage(int err, std::string_view errstr)
{
    std::stringstream s;
    s << errstr << " (" << err << ')';
    return s.str();
}

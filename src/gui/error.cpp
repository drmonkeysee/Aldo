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

auto build_sdl_error(std::string_view message)
{
    std::string errorText{message};
    errorText += " (";
    errorText += SDL_GetError();
    errorText += ')';
    return errorText;
}

auto build_display_what(std::string_view title, std::string_view message)
{
    std::string what{title};
    what += ": ";
    what += message;
    return what;
}

}

//
// Public Interface
//

aldo::SdlError::SdlError(std::string_view message)
: std::runtime_error{build_sdl_error(message)} {}

aldo::DisplayError::DisplayError(std::string_view title,
                                 std::string_view message)
: std::runtime_error{std::string{title}}, msg{message},
    wht{build_display_what(title, message)} {}

aldo::DisplayError::DisplayError(std::string_view title,
                                 std::string_view errnoLabel,int errnoVal)
: DisplayError{title, errnoMessage(errnoLabel, errnoVal)} {}

//
// Private Interface
//

std::string aldo::DisplayError::errnoMessage(std::string_view label, int err)
{
    std::stringstream s;
    s << label << " [" << strerror(err) << " (" << err << ")]";
    return s.str();
}

std::string aldo::DisplayError::emuErrMessage(int err, std::string_view errstr)
{
    std::stringstream s;
    s << errstr << " (" << err << ')';
    return s.str();
}

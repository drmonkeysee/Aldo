//
//  error.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/6/22.
//

#include "error.hpp"

#include <SDL3/SDL.h>

#include <string_view>
#include <cstring>

namespace
{

auto build_sdl_error(std::string& message)
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

auto errno_message(std::string& label, int err)
{
    label += " [";
    label += std::strerror(err);
    label += " (";
    label += std::to_string(err);
    label += ")]";
    return label;
}

}

//
// MARK: - Public Interface
//

aldo::SdlError::SdlError(std::string message)
: std::runtime_error{build_sdl_error(message)} {}

aldo::AldoError::AldoError(std::string title, std::string message)
: std::runtime_error{title}, msg{std::move(message)},
    wht{build_display_what(title, msg)} {}

aldo::AldoError::AldoError(std::string title, std::string label, int errnoVal)
: AldoError{std::move(title), errno_message(label, errnoVal)} {}

//
// MARK: - Private Interface
//

std::string aldo::AldoError::errMessage(int err, std::string&& errstr)
{
    errstr += " (";
    errstr += std::to_string(err);
    errstr += ')';
    return errstr;
}

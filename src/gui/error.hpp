//
//  error.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/6/22.
//

#ifndef Aldo_gui_error_hpp
#define Aldo_gui_error_hpp

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace aldo
{

class SdlError final : public std::runtime_error {
public:
    explicit SdlError(std::string message);
};

class DisplayError final : public std::runtime_error {
public:
    DisplayError(std::string title, std::string message);
    DisplayError(std::string title, std::string_view label, int errnoVal);
    template <typename F>
    DisplayError(std::string title, int err, F errResolver)
    : DisplayError{std::move(title), emuErrMessage(err, errResolver(err))} {}

    const char* what() const noexcept override { return wht.c_str(); }
    const char* title() const noexcept { return std::runtime_error::what(); }
    const char* message() const noexcept { return msg.c_str(); }

private:
    static std::string emuErrMessage(int, std::string_view);

    std::string msg;
    std::string wht;
};

}

#endif

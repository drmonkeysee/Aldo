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

namespace aldo
{

class SdlError final : public std::runtime_error {
public:
    explicit SdlError(std::string_view message);
};

class DisplayError final : public std::runtime_error {
public:
    DisplayError(std::string_view title, std::string_view message);
    DisplayError(std::string_view title, std::string_view errnoLabel,
                 int errnoVal);
    template <typename F>
    DisplayError(std::string_view title, int err, F errResolver)
    : DisplayError{title, emuErrMessage(err, errResolver(err))} {}

    const char* what() const noexcept override { return wht.c_str(); }
    const char* title() const noexcept { return std::runtime_error::what(); }
    const char* message() const noexcept { return msg.c_str(); }

private:
    static std::string errnoMessage(std::string_view label, int err);
    static std::string emuErrMessage(int err, std::string_view errstr);

    std::string msg;
    std::string wht;
};

}

#endif

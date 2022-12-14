//
//  error.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/6/22.
//

#ifndef Aldo_gui_error_hpp
#define Aldo_gui_error_hpp

#include <concepts>
#include <stdexcept>
#include <string>
#include <utility>

namespace aldo
{

template<typename F>
concept ErrConverter = requires(F f) {
    { f(0) } -> std::convertible_to<std::string>;
};

class SdlError final : public std::runtime_error {
public:
    explicit SdlError(std::string message);
};

class AldoError final : public std::runtime_error {
public:
    AldoError(std::string title, std::string message);
    AldoError(std::string title, std::string label, int errnoVal);
    template<ErrConverter F>
    AldoError(std::string title, int err, F errConv)
    : AldoError{std::move(title), errMessage(err, errConv(err))} {}

    const char* what() const noexcept override { return wht.c_str(); }
    const char* title() const noexcept { return std::runtime_error::what(); }
    const char* message() const noexcept { return msg.c_str(); }

private:
    static std::string errMessage(int, std::string&&);

    std::string msg;
    std::string wht;
};

}

#endif

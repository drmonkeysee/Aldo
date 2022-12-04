//
//  emulator.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/4/22.
//

#ifndef Aldo_gui_emulator_hpp
#define Aldo_gui_emulator_hpp

#include "cart.h"
#include "debug.h"
#include "guiplatform.h"
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <string>
#include <string_view>

namespace aldo
{

struct event;
struct viewstate;

using cart_handle = handle<cart, cart_free>;
using console_handle = handle<nes, nes_free>;
using debug_handle = handle<debugctx, debug_free>;

class EmuController final {
public:
    // TODO: make ownership transfer clear
    EmuController(debugctx& d, nes& n) noexcept : hdebug{&d}, hconsole{&n} {}

    std::string_view cartName() const;

    void handleInput(viewstate& state, const gui_platform& platform);
    void update(viewstate& state, console_state& snapshot) const noexcept;

private:
    void loadCartFrom(std::string_view);
    void openCartFile(const gui_platform&);
    void processEvent(const event&, viewstate&, const gui_platform&);

    std::string cartFilepath;
    debug_handle hdebug;
    cart_handle hcart;
    console_handle hconsole;
};

}

#endif

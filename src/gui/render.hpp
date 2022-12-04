//
//  render.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_render_hpp
#define Aldo_gui_render_hpp

#include "snapshot.h"

namespace aldo
{

class EmuController;
class MediaRuntime;
struct viewstate;

class RenderFrame final {
public:
    explicit RenderFrame(const MediaRuntime& r) noexcept;
    RenderFrame(MediaRuntime&&) = delete;
    RenderFrame(const RenderFrame&) = delete;
    RenderFrame& operator=(const RenderFrame&) = delete;
    RenderFrame(RenderFrame&&) = delete;
    RenderFrame& operator=(RenderFrame&&) = delete;
    ~RenderFrame();

    void render(viewstate& state, const EmuController& controller,
                const console_state& snapshot) const;

private:
    void renderMainMenu(viewstate&) const noexcept;
    void renderHardwareTraits(viewstate&, const console_state&) const noexcept;
    void renderCart(const EmuController&, const console_state&) const;
    void renderPrg(const console_state&) const noexcept;
    void renderBouncer(viewstate&) const noexcept;
    void renderCpu(viewstate&, const console_state&) const noexcept;
    void renderRam(const console_state&) const noexcept;

    const MediaRuntime& runtime;
};

}

#endif

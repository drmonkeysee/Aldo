//
//  render.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_render_hpp
#define Aldo_gui_render_hpp

#include "nes.h"
#include "snapshot.h"

namespace aldo
{

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

    void render(viewstate& state, nes* console,
                const console_state& snapshot) const noexcept;

private:
    void renderMainMenu(viewstate& state) const noexcept;
    void renderHardwareTraits(nes* console,
                              const console_state& snapshot) const noexcept;
    void renderCart(const console_state& snapshot) const noexcept;
    void renderBouncer(viewstate& state) const noexcept;
    void renderCpu(viewstate& state,
                   const console_state& snapshot) const noexcept;
    void renderRam(const console_state& snapshot) const noexcept;

    const MediaRuntime& runtime;
};

}

#endif

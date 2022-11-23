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

class MediaRuntime;
struct viewstate;

class RenderFrame final {
public:
    RenderFrame(viewstate& s, const MediaRuntime& r,
                const console_state& cs) noexcept;
    RenderFrame(viewstate&, MediaRuntime&&, console_state&&) = delete;
    RenderFrame(viewstate&, const MediaRuntime&, console_state&&) = delete;
    RenderFrame(viewstate&, MediaRuntime&&, const console_state&) = delete;
    RenderFrame(const RenderFrame&) = delete;
    RenderFrame& operator=(const RenderFrame&) = delete;
    RenderFrame(RenderFrame&&) = delete;
    RenderFrame& operator=(RenderFrame&&) = delete;
    ~RenderFrame();

    void render() const noexcept;

private:
    void renderMainMenu() const noexcept;
    void renderHardwareTraits() const noexcept;
    void renderCart() const noexcept;
    void renderBouncer() const noexcept;
    void renderCpu() const noexcept;
    void renderRam() const noexcept;

    viewstate& state;
    const MediaRuntime& runtime;
    const console_state& snapshot;
};

}

#endif

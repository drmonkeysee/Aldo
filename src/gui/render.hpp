//
//  render.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_render_hpp
#define Aldo_gui_render_hpp

#include "runclock.hpp"

namespace aldo
{

class MediaRuntime;

class RenderFrame final {
public:
    RenderFrame(const MediaRuntime& r, RunTimer t) noexcept;
    RenderFrame(MediaRuntime&&, RunTimer) = delete;
    RenderFrame(const RenderFrame&) = delete;
    RenderFrame& operator=(const RenderFrame&) = delete;
    RenderFrame(RenderFrame&&) = delete;
    RenderFrame& operator=(RenderFrame&&) = delete;
    ~RenderFrame();

private:
    const MediaRuntime& r;
    RunTimer t;
};

}

#endif

//
//  render.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#ifndef Aldo_gui_render_hpp
#define Aldo_gui_render_hpp

#include "mediaruntime.hpp"
#include "viewstate.hpp"

namespace aldo
{

class RenderFrame final {
public:
    RenderFrame(viewstate& s, const MediaRuntime& r) noexcept;
    RenderFrame(viewstate& s, MediaRuntime&& r) = delete;
    RenderFrame(const RenderFrame&) = delete;
    RenderFrame& operator=(const RenderFrame&) = delete;
    RenderFrame(RenderFrame&&) = delete;
    RenderFrame& operator=(RenderFrame&&) = delete;
    ~RenderFrame();

    void render() const noexcept;

private:
    void renderBouncer() const noexcept;

    viewstate& state;
    const MediaRuntime& runtime;
};

}

#endif

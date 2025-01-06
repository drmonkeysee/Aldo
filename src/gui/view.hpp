//
//  view.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/10/22.
//

#ifndef Aldo_gui_view_hpp
#define Aldo_gui_view_hpp

#include "imgui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace aldo
{

class Emulator;
class MediaRuntime;
struct viewstate;

class View {
public:
    enum class Transition {
        none,
        open,
        close,
        expand,
        collapse,
    };

    class TransitionLatch {
    public:
        TransitionLatch() noexcept {}
        TransitionLatch(Transition t) noexcept : transition{t} {}

        // NOTE: exchange is becoming noexcept in C++23
        // but exchanging an enum won't throw anyway.
        Transition reset() noexcept
        {
            return std::exchange(transition, Transition::none);
        }

    private:
        Transition transition = Transition::none;
    };

    View(std::string title, viewstate& vs, const Emulator& emu,
         const MediaRuntime& mr,
         ImGuiWindowFlags options = ImGuiWindowFlags_None) noexcept
    : vs{vs}, emu{emu}, mr{mr}, title{std::move(title)}, options{options} {}
    View(std::string, viewstate&, Emulator&&, const MediaRuntime&,
         ImGuiWindowFlags) = delete;
    View(std::string, viewstate&, const Emulator&&, MediaRuntime&&,
         ImGuiWindowFlags) = delete;
    View(std::string, viewstate&, Emulator&&, MediaRuntime&&,
         ImGuiWindowFlags) = delete;
    View(const View&) = delete;
    View& operator=(const View&) = delete;
    View(View&&) = delete;
    View& operator=(View&&) = delete;
    virtual ~View() = default;

    const std::string& windowTitle() const noexcept { return title; }
    bool* visibility() noexcept { return &visible; }

    void render();

    TransitionLatch transition;

protected:
    virtual void renderContents() = 0;

    viewstate& vs;
    const Emulator& emu;
    const MediaRuntime& mr;

private:
    void handleTransition() noexcept;

    std::string title;
    ImGuiWindowFlags options;
    bool visible = true;    // TODO: get this from imgui settings
};

class Layout {
public:
    using view_list = std::vector<std::unique_ptr<View>>;

    Layout(viewstate& vs, const Emulator& emu, const MediaRuntime& mr);
    Layout(viewstate&, Emulator&&, const MediaRuntime&) = delete;
    Layout(viewstate&, const Emulator&, MediaRuntime&&) = delete;
    Layout(viewstate&, Emulator&&, MediaRuntime&&) = delete;

    void render() const;

private:
    viewstate& vs;
    const Emulator& emu;
    const MediaRuntime& mr;
    view_list views;
};

}

#endif

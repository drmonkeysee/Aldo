//
//  view.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/10/22.
//

#ifndef Aldo_gui_view_hpp
#define Aldo_gui_view_hpp

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
    View(std::string title, viewstate& vs, const Emulator& emu,
         const MediaRuntime& mr) noexcept
    : title{std::move(title)}, vs{vs}, emu{emu}, mr{mr} {}
    View(std::string, viewstate&, Emulator&&, const MediaRuntime&) = delete;
    View(std::string, viewstate&, const Emulator&&, MediaRuntime&&) = delete;
    View(std::string, viewstate&, Emulator&&, MediaRuntime&&) = delete;
    View(const View&) = delete;
    View& operator=(const View&) = delete;
    View(View&&) = delete;
    View& operator=(View&&) = delete;
    virtual ~View() = default;

    const std::string& windowTitle() const noexcept { return title; }
    bool* visibility() noexcept { return &visible; }

    void render();

protected:
    virtual void renderContents() = 0;

    std::string title;
    viewstate& vs;
    const Emulator& emu;
    const MediaRuntime& mr;
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

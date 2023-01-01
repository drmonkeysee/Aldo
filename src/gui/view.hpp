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

class EmuController;
class MediaRuntime;
struct viewstate;

class View {
public:
    View(std::string title, viewstate& s, const EmuController& c,
         const MediaRuntime& r, bool* v = nullptr) noexcept
    : title{std::move(title)}, s{s}, c{c}, r{r}, visible{v} {}
    View(std::string, viewstate&, EmuController&&,
         const MediaRuntime&, bool*) = delete;
    View(std::string, viewstate&, const EmuController&&,
         MediaRuntime&&, bool*) = delete;
    View(std::string, viewstate&, EmuController&&, MediaRuntime&&,
         bool*) = delete;
    virtual ~View() = default;

    void render();

protected:
    virtual void renderContents() = 0;

    std::string title;
    viewstate& s;
    const EmuController& c;
    const MediaRuntime& r;
    bool* visible;
};

class Layout final {
public:
    Layout(viewstate& s, const EmuController& c, const MediaRuntime& r);
    Layout(viewstate&, EmuController&&, const MediaRuntime&) = delete;
    Layout(viewstate&, const EmuController&, MediaRuntime&&) = delete;
    Layout(viewstate&, EmuController&&, MediaRuntime&&) = delete;

    void render() const;

private:
    viewstate& s;
    const MediaRuntime& r;
    std::vector<std::unique_ptr<View>> views;
};

}

#endif

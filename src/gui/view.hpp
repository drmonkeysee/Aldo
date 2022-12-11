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
         const MediaRuntime& r) noexcept
    : title{std::move(title)}, s{s}, c{c}, r{r} {}
    View(std::string, viewstate&, EmuController&&,
         const MediaRuntime&) = delete;
    View(std::string, viewstate&, const EmuController&&,
         MediaRuntime&&) = delete;
    View(std::string, viewstate&, EmuController&&, MediaRuntime&&) = delete;
    virtual ~View() = default;

    void render() const;

protected:
    virtual void renderContents() const = 0;
    std::string title;
    viewstate& s;
    const EmuController& c;
    const MediaRuntime& r;
};

class Layout final {
public:
    Layout(viewstate& s, const EmuController& c, const MediaRuntime& r);
    Layout(viewstate& s, EmuController&& c, const MediaRuntime& r) = delete;
    Layout(viewstate& s, const EmuController& c, MediaRuntime&& r) = delete;
    Layout(viewstate& s, EmuController&& c, MediaRuntime&& r) = delete;

    void render() const;

private:
    viewstate& s;
    const MediaRuntime& r;
    std::vector<std::unique_ptr<View>> views;
};

}

#endif

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
#include "handle.hpp"
#include "nes.h"
#include "snapshot.h"

#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <cstddef>

struct gui_platform;

namespace aldo
{

struct event;
struct viewstate;

using cart_handle = handle<cart, cart_free>;
using console_handle = handle<nes, nes_free>;
using debug_handle = handle<debugctx, debug_free>;

class SnapshotScope final {
public:
    explicit SnapshotScope(nes* console) noexcept
    {
        nes_snapshot(console, getp());
    }
    ~SnapshotScope() { snapshot_clear(getp()); }

    const console_state& get() const noexcept { return snapshot; }
    const console_state* getp() const noexcept { return &snapshot; }
    console_state* getp() noexcept { return &snapshot; }

private:
    console_state snapshot;
};

class BreakpointIterator final {
public:
    using difference_type = std::ptrdiff_t;
    using value_type = breakpoint;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = iterator_category;

    BreakpointIterator() noexcept {}
    explicit BreakpointIterator(debugctx* db) noexcept : debugger{db} {}

    void swap(BreakpointIterator& that) noexcept
    {
        auto tmp{that};
        that = *this;
        *this = tmp;
    }

    reference operator*() const noexcept { return *get_breakpoint(); }
    pointer operator->() const noexcept { return get_breakpoint(); }
    BreakpointIterator& operator++() noexcept { ++idx; return *this; }
    BreakpointIterator operator++(int) noexcept
    {
        auto tmp{*this};
        ++*this;
        return tmp;
    }
    friend bool operator==(const BreakpointIterator& a,
                           const BreakpointIterator& b)
    {
        // NOTE: default ctor iterator is equivalent to end()
        if (!a.debugger) return b.idx
                            == static_cast<difference_type>(b.count());
        if (!b.debugger) return a.idx
                            == static_cast<difference_type>(a.count());
        return a.debugger == b.debugger && a.idx == b.idx;
    }
    friend bool operator!=(const BreakpointIterator& a,
                           const BreakpointIterator& b)
    {
        return !(a == b);
    }

private:
    pointer get_breakpoint() const noexcept
    {
        if (!debugger) return nullptr;
        return debug_bp_at(debugger, idx);
    }

    std::size_t count() const noexcept
    {
        if (!debugger) return 0;
        return debug_bp_count(debugger);
    }

    debugctx* debugger = nullptr;
    mutable difference_type idx = 0;
};

inline void swap(BreakpointIterator& a, BreakpointIterator& b) noexcept
{
    a.swap(b);
}

class BreakpointsProxy final {
public:
    using iterator = BreakpointIterator;
    using const_iterator = const iterator;

    explicit BreakpointsProxy(debugctx* db) noexcept : debugger{db} {}

    iterator begin() noexcept { return iterator{debugger}; }
    iterator end() noexcept { return iterator{}; }
    const_iterator cbegin() const noexcept { return const_iterator{debugger}; }
    const_iterator cend() const noexcept { return const_iterator{}; }

private:
    static_assert(std::forward_iterator<BreakpointIterator>,
                  "Breakpoint Iterator definition incomplete");
    debugctx* debugger;
};

class EmuController final {
public:
    EmuController(debug_handle d, console_handle n) noexcept
    : hdebug{std::move(d)}, hconsole{std::move(n)},
        lsnapshot{consolep()} {}

    std::string_view cartName() const;
    std::optional<cartinfo> cartInfo() const;
    const console_state& snapshot() const noexcept { return lsnapshot.get(); }
    const console_state* snapshotp() const noexcept {
        return lsnapshot.getp();
    }
    BreakpointsProxy breakpoints() const noexcept
    {
        return BreakpointsProxy{debugp()};
    }

    void handleInput(viewstate& state, const gui_platform& platform);
    void update(viewstate& state) noexcept;

private:
    debugctx* debugp() const noexcept { return hdebug.get(); }
    cart* cartp() const noexcept { return hcart.get(); }
    nes* consolep() const noexcept { return hconsole.get(); }
    console_state* snapshotp() noexcept { return lsnapshot.getp(); }

    void loadCartFrom(const char*);
    void openCartFile(const gui_platform&);
    void processEvent(const event&, viewstate&, const gui_platform&);
    void updateBouncer(viewstate&) const noexcept;

    std::string cartFilepath;
    debug_handle hdebug;
    cart_handle hcart;
    console_handle hconsole;
    SnapshotScope lsnapshot;
};

}

#endif

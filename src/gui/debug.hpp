//
//  debug.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/27/23.
//

#ifndef Aldo_gui_debug_hpp
#define Aldo_gui_debug_hpp

#include "debug.h"
#include "emutypes.hpp"
#include "haltexpr.h"
#include "handle.hpp"

#include <filesystem>
#include <iterator>
#include <utility>

namespace aldo
{

using debug_handle = handle<debugctx, debug_free>;

class Debugger {
public:
    class BreakpointsView;
    using bp_collection = BreakpointsView;

    explicit Debugger(debug_handle d) noexcept
    : hdebug{std::move(d)}, bpview{debugp()} {}

    void vectorOverride(int resetvector) noexcept
    {
        debug_set_vector_override(debugp(), resetvector);
    }

    const bp_collection& breakpoints() const noexcept { return bpview; }
    bp_collection& breakpoints() noexcept { return bpview; }

    bool isActive() const noexcept
    {
        return vectorOverride() != NoResetVector || !breakpoints().empty();
    }

    void loadBreakpoints(const std::filesystem::path& filepath);
    void exportBreakpoints(const std::filesystem::path& filepath) const;

    // NOTE: non-owning view over the debugger's breakpoints collection
    class BreakpointsView {
    public:
        using value_type = breakpoint;
        using size_type = et::size;
        using difference_type = et::diff;
        using const_reference = const value_type&;
        using const_pointer = const value_type*;
        class BreakpointIterator;
        using const_iterator = BreakpointIterator;

        explicit BreakpointsView(debugctx* d) noexcept : debugp{d} {}

        size_type size() const noexcept { return debug_bp_count(debugp); }
        bool empty() const noexcept { return size() == 0; }
        const_pointer at(difference_type i) const noexcept
        {
            return debug_bp_at(debugp, i);
        }

        void append(haltexpr expr) noexcept { debug_bp_add(debugp, expr); }
        void toggleEnabled(difference_type i) noexcept
        {
            const auto bp = at(i);
            debug_bp_enabled(debugp, i, bp && !bp->enabled);
        }
        void remove(difference_type i) noexcept { debug_bp_remove(debugp, i); }
        void clear() noexcept { debug_bp_clear(debugp); }

        class BreakpointIterator {
        public:
            using difference_type = BreakpointsView::difference_type;
            using value_type = BreakpointsView::value_type;
            using pointer = BreakpointsView::const_pointer;
            using reference = BreakpointsView::const_reference;
            using iterator_concept = std::input_iterator_tag;

            explicit BreakpointIterator(debugctx* d) noexcept : debugp{d} {}

            reference operator*() const noexcept
            {
                // NOTE: always guard with end() iterator
                // to avoid dereferencing null.
                return *debug_bp_at(debugp, idx);
            }

            BreakpointIterator& operator++() noexcept { ++idx; return *this; }
            BreakpointIterator operator++(int) noexcept
            {
                auto tmp = *this;
                ++*this;
                return tmp;
            }
            void swap(BreakpointIterator& that) noexcept
            {
                using std::swap;
                swap(debugp, that.debugp);
                swap(idx, that.idx);
            }

        private:
            debugctx* debugp;
            difference_type idx = 0;
        };

    private:
        static_assert(std::input_iterator<const_iterator>,
                      "Incomplete breakpoint iterator definition");

        debugctx* debugp;
    };

private:
    int vectorOverride() const noexcept
    {
        return debug_vector_override(debugp());
    }

    debugctx* debugp() const noexcept { return hdebug.get(); }

    debug_handle hdebug;
    bp_collection bpview;
};

inline void swap(Debugger::BreakpointsView::BreakpointIterator& a,
                 Debugger::BreakpointsView::BreakpointIterator& b) noexcept
{
    a.swap(b);
}

}

#endif

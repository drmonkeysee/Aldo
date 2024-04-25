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
#include "snapshot.h"

#include <filesystem>
#include <iterator>
#include <string>
#include <utility>

namespace aldo
{

namespace debug
{

inline constexpr const char* BreakFileExtension = "brk";

inline std::filesystem::path breakfile_path_from(std::filesystem::path path)
{
    if (path.empty()) {
        path = "breakpoints";
    }
    return path.replace_extension(BreakFileExtension);
}

}

using debug_handle = handle<debugctx, debug_free>;

class Debugger {
public:
    template<bool> class BreakpointsView;
    using BpView = BreakpointsView<false>;
    using MutableBpView = BreakpointsView<true>;

    int vectorOverride() const noexcept
    {
        return debug_vector_override(debugp());
    }
    void vectorOverride(int resetvector) noexcept
    {
        debug_set_vector_override(debugp(), resetvector);
    }
    void vectorClear() noexcept
    {
        vectorOverride(NoResetVector);
    }
    bool isVectorOverridden() const noexcept
    {
        return vectorOverride() != NoResetVector;
    }
    BpView breakpoints() const noexcept { return debugp(); }
    MutableBpView breakpoints() noexcept { return debugp(); }
    bool isActive() const noexcept
    {
        return isVectorOverridden() || !breakpoints().empty();
    }

    void loadBreakpoints(const std::filesystem::path& filepath);
    void exportBreakpoints(const std::filesystem::path& filepath) const;
    void loadCartState(const std::filesystem::path& prefCartPath);
    void saveCartState(const std::filesystem::path& prefCartPath) const;

    // NOTE: as usual an iterator is invalidated if the underlying
    // collection is modified.
    class BreakpointIterator {
    public:
        using difference_type = et::diff;
        using value_type = breakpoint;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_concept = std::forward_iterator_tag;

        BreakpointIterator() noexcept = default;
        BreakpointIterator(debugctx* d, difference_type count) noexcept
        : debugp{d}, count{count} {}

        reference operator*() const noexcept
        {
            return *operator->();
        }
        pointer operator->() const noexcept
        {
            return debug_bp_at(debugp, idx);
        }

        BreakpointIterator& operator++() noexcept { ++idx; return *this; }
        BreakpointIterator operator++(int) noexcept
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend bool operator==(const BreakpointIterator& a,
                               const BreakpointIterator& b) noexcept
        {
            return (a.sentinel() && b.sentinel())
                    || (b.sentinel() && a.exhausted())
                    || (a.sentinel() && b.exhausted())
                    || (a.debugp == b.debugp && a.idx == b.idx
                        && a.count == b.count);
        }

    private:
        bool sentinel() const noexcept { return !debugp; }
        bool exhausted() const noexcept { return idx == count; }

        debugctx* debugp = nullptr;
        difference_type count = 0, idx = 0;
    };

    // NOTE: non-owning view over the debugger's breakpoints collection;
    // uses mutability template to allow value semantics without inadvertently
    // violating const-correctness of the underlying breakpoints C API.
    template<bool Mutable = false>
    class BreakpointsView {
    public:
        using const_iterator = BreakpointIterator;
        using value_type = const_iterator::value_type;
        using size_type = et::size;
        using difference_type = const_iterator::difference_type;
        using const_reference = const_iterator::reference;
        using const_pointer = const_iterator::pointer;

        size_type size() const noexcept { return debug_bp_count(debugp); }
        bool empty() const noexcept { return size() == 0; }
        const_pointer at(difference_type i) const noexcept
        {
            return debug_bp_at(debugp, i);
        }

        const_iterator cbegin() const noexcept
        {
            // NOTE: ssize is not noexcept but all it does is delegate to
            // this->size() which in this case is noexcept.
            return {debugp, std::ssize(*this)};
        }
        const_iterator cend() const noexcept { return {}; }
        const_iterator begin() const noexcept { return cbegin(); }
        const_iterator end() const noexcept { return cend(); }

        void append(haltexpr expr) noexcept requires Mutable
        {
            debug_bp_add(debugp, expr);
        }
        void enable(difference_type i) noexcept requires Mutable
        {
            debug_bp_enabled(debugp, i, true);
        }
        void disable(difference_type i) noexcept requires Mutable
        {
            debug_bp_enabled(debugp, i, false);
        }
        void remove(difference_type i) noexcept requires Mutable
        {
            debug_bp_remove(debugp, i);
        }
        void clear() noexcept requires Mutable { debug_bp_clear(debugp); }

    private:
        static_assert(std::forward_iterator<const_iterator>,
                      "Incomplete breakpoint iterator definition");
        friend Debugger;

        BreakpointsView(debugctx* d) noexcept : debugp{d} {}

        debugctx* debugp;
    };

private:
    friend class Emulator;

    explicit Debugger(debug_handle d) noexcept : hdebug{std::move(d)} {}

    debugctx* debugp() const noexcept { return hdebug.get(); }

    bool hasBreak(const console_state& snapshot) const noexcept
    {
        return snapshot.debugger.halted != NoBreakpoint;
    }

    debug_handle hdebug;
};

}

#endif

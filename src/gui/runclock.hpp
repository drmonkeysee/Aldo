//
//  runclock.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/20/22.
//

#ifndef Aldo_gui_runclock_hpp
#define Aldo_gui_runclock_hpp

#include "attr.hpp"
#include "cycleclock.h"

#include <algorithm>
#include <chrono>
#include <ratio>
#include <utility>

namespace aldo
{

class ALDO_SIDEFX RunTimer {
public:
    using clock_type = std::chrono::steady_clock;
    using duration_type = std::chrono::duration<double, std::milli>;

    explicit RunTimer(double& result) noexcept : result{result} {}
    RunTimer(const RunTimer&) = delete;
    RunTimer& operator=(const RunTimer&) = delete;
    RunTimer(RunTimer&& that) noexcept
    : recorded{that.recorded}, start{that.start}, result{that.result}
    {
        that.recorded = true;
    }
    RunTimer& operator=(RunTimer&& that) noexcept
    {
        if (this == &that) return *this;

        swap(that);
        return *this;
    }
    ~RunTimer() { record(); }

    void record() noexcept
    {
        if (recorded) return;

        auto elapsed = clock_type::now() - start;
        // NOTE: duration_cast is not noexcept but i don't see how converting
        // to a double would ever throw; i'll assume it's noexcept in practice.
        auto converted = std::chrono::duration_cast<duration_type>(elapsed);
        result = converted.count();
        recorded = true;
    }

    void swap(RunTimer& that) noexcept
    {
        using std::swap;

        swap(recorded, that.recorded);
        swap(start, that.start);
        swap(result, that.result);
    }

private:
    bool recorded = false;
    std::chrono::time_point<clock_type> start = clock_type::now();
    double& result;
};

inline void swap(RunTimer& a, RunTimer& b) noexcept
{
    a.swap(b);
}

class ALDO_SIDEFX RunTick {
public:
    RunTick(cycleclock& c, bool resetBudget) noexcept : cyclock{c}
    {
        cycleclock_tickstart(&cyclock, resetBudget);
    }
    RunTick(const RunTick&) = delete;
    RunTick& operator=(const RunTick&) = delete;
    RunTick(RunTick&&) = delete;
    RunTick& operator=(RunTick&&) = delete;
    ~RunTick() { cycleclock_tickend(&cyclock); }

private:
    cycleclock& cyclock;
};

struct runclock {
    void start() noexcept
    {
        cycleclock_start(&cyclock);
    }

    RunTick startTick(bool resetBudget) noexcept
    {
        return RunTick(cyclock, resetBudget);
    }

    RunTimer timeInput() noexcept { return RunTimer{dtInputMs}; }
    RunTimer timeUpdate() noexcept { return RunTimer{dtUpdateMs}; }
    RunTimer timeRender() noexcept { return RunTimer{dtRenderMs}; }

    void adjustCycleRate(int adjustment) noexcept
    {
        auto adjusted = cyclock.cpf + adjustment;
        cyclock.cpf = std::max(MinCpf, std::min(adjusted, MaxCpf));
    }

    cycleclock cyclock{.cpf = 10, .fps = MinFps};
    double dtInputMs = 0, dtUpdateMs = 0, dtRenderMs = 0;
};

}

#endif

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
#include "nes.h"

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

class RunClock {
public:
    double dtInputMs() noexcept { return dtInput; }
    double dtUpdateMs() noexcept { return dtUpdate; }
    double dtRenderMs() noexcept { return dtRender; }
    cyclkscale scale() noexcept { return currentScale; }
    cycleclock& cyclock() noexcept { return clock; }
    cycleclock* cyclockp() noexcept { return &clock; }

    void start() noexcept
    {
        cycleclock_start(cyclockp());
    }

    RunTick startTick(bool resetBudget) noexcept
    {
        return {cyclock(), resetBudget};
    }

    void resetEmu() noexcept {
        cyclock().emutime = 0;
        cyclock().cycles = cyclock().frames = cyclock().subcycle = 0;
    }

    RunTimer timeInput() noexcept { return RunTimer{dtInput}; }
    RunTimer timeUpdate() noexcept { return RunTimer{dtUpdate}; }
    RunTimer timeRender() noexcept { return RunTimer{dtRender}; }

    void adjustCycleRate(int adjustment) noexcept
    {
        auto adjusted = cyclock().rate + adjustment;
        cyclock().rate = std::max(Aldo_MinCps,
                                  std::min(adjusted, Aldo_MaxCps));
    }

    void setScale(cyclkscale s) noexcept
    {
        if (s == scale()) return;

        auto prev = oldRate;
        oldRate = cyclock().rate;
        cyclock().rate = prev;
        currentScale = s;
        cyclock().rate_factor = currentScale == CYCS_CYCLE
                                ? nes_cycle_factor()
                                : nes_frame_factor();
    }

    void toggleScale() noexcept
    {
        setScale(static_cast<cyclkscale>(!scale()));
    }

private:
    cycleclock clock{.rate = 10, .rate_factor = nes_cycle_factor()};
    double dtInput = 0, dtUpdate = 0, dtRender = 0;
    cyclkscale currentScale = CYCS_CYCLE;
    int oldRate = Aldo_MinFps;
};

}

#endif

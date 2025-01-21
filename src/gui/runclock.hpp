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
    RunTick(aldo_clock& c, bool resetBudget) noexcept : clock{c}
    {
        aldo_clock_tickstart(&clock, resetBudget);
    }
    RunTick(const RunTick&) = delete;
    RunTick& operator=(const RunTick&) = delete;
    RunTick(RunTick&&) = delete;
    RunTick& operator=(RunTick&&) = delete;
    ~RunTick() { aldo_clock_tickend(&clock); }

private:
    aldo_clock& clock;
};

class RunClock {
public:
    double dtInputMs() const noexcept { return dtInput; }
    double dtUpdateMs() const noexcept { return dtUpdate; }
    double dtRenderMs() const noexcept { return dtRender; }
    aldo_clockscale scale() const noexcept { return currentScale; }
    // TODO: use deducing this when it's finally supported
    const aldo_clock& clock() const noexcept { return clk; }
    const aldo_clock* clockp() const noexcept { return &clk; }
    aldo_clock& clock() noexcept { return clk; }
    aldo_clock* clockp() noexcept { return &clk; }

    bool atMinRate() const noexcept
    {
        return atRateLimit(Aldo_MinCps, Aldo_MinFps);
    }

    bool atMaxRate() const noexcept
    {
        return atRateLimit(Aldo_MaxCps, Aldo_MaxFps);
    }

    void start() noexcept { aldo_clock_start(clockp()); }

    RunTick startTick(bool resetBudget) noexcept
    {
        return {clock(), resetBudget};
    }

    void resetEmu() noexcept
    {
        clock().emutime = 0;
        clock().cycles = clock().frames = clock().subcycle = 0;
    }

    RunTimer timeInput() noexcept { return RunTimer{dtInput}; }
    RunTimer timeUpdate() noexcept { return RunTimer{dtUpdate}; }
    RunTimer timeRender() noexcept { return RunTimer{dtRender}; }

    void adjustRate(int adjustment) noexcept
    {
        auto adjusted = clock().rate + adjustment;
        int min, max;
        if (currentScale == ALDO_CS_CYCLE) {
            min = Aldo_MinCps;
            max = Aldo_MaxCps;
        } else {
            min = Aldo_MinFps;
            max = Aldo_MaxFps;
        }
        clock().rate = std::max(min, std::min(adjusted, max));
    }

    void setScale(aldo_clockscale s) noexcept
    {
        if (s == currentScale) return;

        auto prev = oldRate;
        oldRate = clock().rate;
        clock().rate = prev;
        currentScale = s;
        clock().rate_factor = currentScale == ALDO_CS_CYCLE
                                ? aldo_nes_cycle_factor()
                                : aldo_nes_frame_factor();
    }

    void toggleScale() noexcept
    {
        setScale(static_cast<aldo_clockscale>(!currentScale));
    }

private:
    bool atRateLimit(int cycleLimit, int frameLimit) const noexcept
    {
        return currentScale == ALDO_CS_CYCLE
                ? clock().rate == cycleLimit
                : clock().rate == frameLimit;
    }

    aldo_clock clk{
        .rate = Aldo_MaxFps,
        .rate_factor = aldo_nes_frame_factor(),
    };
    double dtInput = 0, dtUpdate = 0, dtRender = 0;
    aldo_clockscale currentScale = ALDO_CS_FRAME;
    int oldRate = 10;
};

}

#endif

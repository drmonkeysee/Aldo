//
//  runclock.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/20/22.
//

#ifndef Aldo_gui_runclock_hpp
#define Aldo_gui_runclock_hpp

#include "cycleclock.h"

#include <chrono>
#include <ratio>

namespace aldo
{

class RunTimer final {
public:
    using clock_type = std::chrono::steady_clock;
    using duration_type = std::chrono::duration<double, std::milli>;

    explicit RunTimer(double& result) noexcept : result{result} {}

    RunTimer(const RunTimer&) = delete;
    RunTimer& operator=(const RunTimer&) = delete;

    RunTimer(RunTimer&& that) noexcept
    : start{that.start}, result{that.result}
    {
        that.recorded = true;
    }
    RunTimer& operator=(RunTimer&&) = delete;

    ~RunTimer()
    {
        if (!recorded) {
            record();
        }
    }

    void record() noexcept
    {
        const auto elapsed = clock_type::now() - start;
        const auto converted =
            std::chrono::duration_cast<duration_type>(elapsed);
        result = converted.count();
        recorded = true;
    }

private:
    bool recorded = false;
    std::chrono::time_point<clock_type> start = clock_type::now();
    double& result;
};

struct runclock {
    void start() noexcept
    {
        cycleclock_start(&cyclock);
    }

    void tickStart(bool resetBudget) noexcept
    {
        cycleclock_tickstart(&cyclock, resetBudget);
    }

    void tickEnd() noexcept
    {
        cycleclock_tickend(&cyclock);
    }

    RunTimer timeInput() noexcept { return RunTimer{dtInputMs}; }
    RunTimer timeUpdate() noexcept { return RunTimer{dtUpdateMs}; }
    RunTimer timeRender() noexcept { return RunTimer{dtRenderMs}; }

    cycleclock cyclock{.cycles_per_sec = 4};
    double dtInputMs = 0, dtUpdateMs = 0, dtRenderMs = 0;
};

}

#endif

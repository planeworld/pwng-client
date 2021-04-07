#ifndef PERFORMANCE_TIMERS_HPP
#define PERFORMANCE_TIMERS_HPP

#include "avg_filter.hpp"
#include "timer.hpp"

struct PerformanceTimers
{
    Timer Queue;
    Timer Render;
    Timer ViewportTest;

    AvgFilter<double> QueueAvg{200};
    AvgFilter<double> RenderAvg{100};
    AvgFilter<double> ViewportTestAvg{100};
};

#endif // PERFORMANCE_TIMERS_HPP

#ifndef PERFORMANCE_TIMERS_HPP
#define PERFORMANCE_TIMERS_HPP

#include "avg_filter.hpp"
#include "timer.hpp"

struct PerformanceTimers
{
    Timer Queue;
    Timer Render;
    Timer ViewportTest;

    AvgFilter<double> QueueAvg{100};
    AvgFilter<double> RenderAvg{50};
    AvgFilter<double> ViewportTestAvg{50};

    AvgFilter<double> ServerPhysicsFrameTimeAvg{50};
    AvgFilter<double> ServerQueueInFrameTimeAvg{50};
    AvgFilter<double> ServerQueueOutFrameTimeAvg{50};
    AvgFilter<double> ServerSimFrameTimeAvg{50};
};

#endif // PERFORMANCE_TIMERS_HPP

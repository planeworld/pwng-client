#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

using namespace std::chrono;
using HiResClock = high_resolution_clock;

class Timer
{
    public:
        Timer() {this->start();}
        void    start()        {Start = HiResClock::now();}
        void    stop()         {Stop = HiResClock::now();}
        void    restart()      {stop(); start();}
        double  elapsed() const     {return duration<double>(Stop-Start).count();}
        double  elapsed_s() const   {return duration<double>(Stop-Start).count();}
        double  elapsed_ms() const  {return duration<double>(Stop-Start).count()*1000.0;}
        double  elapsed_us() const  {return duration<double>(Stop-Start).count()*1.0e6;}
        double  split() const       {return duration<double>(HiResClock::now() - Start).count();}
        double  split_s() const     {return duration<double>(HiResClock::now() - Start).count();}
        double  split_ms() const    {return duration<double>(HiResClock::now() - Start).count()*1000.0;}
        double  split_us() const    {return duration<double>(HiResClock::now() - Start).count()*1.0e6;}
        double  time() const        {return duration<double>(HiResClock::now() - Start).count();}
        double  time_s() const      {return duration<double>(HiResClock::now() - Start).count();}
        double  time_ms() const     {return duration<double>(HiResClock::now() - Start).count()*1000.0;}
        double  time_us() const     {return duration<double>(HiResClock::now() - Start).count()*1.0e6;}

    private:
        
        HiResClock::time_point Start;
        HiResClock::time_point Stop;
};

#endif // TIMER_HPP

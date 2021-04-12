#ifndef AVG_FILTER_HPP
#define AVG_FILTER_HPP

// Use a double ended queue internally.
// In principal, only std::queue is needed, but since queue is just an adaptor
// to deque we might as well directly use a deque here
#include <deque>
#include <numeric>
#include <iostream>

template<class T>
class AvgFilter
{
    
    public:

        AvgFilter() = delete;
        explicit AvgFilter(std::size_t _s) : BufferSize_(_s){}

        void addValue(T _v)
        {
            if (Values_.size() == BufferSize_)
            {
                Values_.pop_front();
            }
            Values_.push_back(_v);
        }

        T getAvg() const
        {
            T Avg = std::accumulate(Values_.cbegin(), Values_.cend(), 0.0);
            if (Values_.size() > 0)
                return Avg / Values_.size();
            else
                return 0;
        }

        T getAvg_ms() const
        {
            T Avg = std::accumulate(Values_.cbegin(), Values_.cend(), 0.0);
            if (Values_.size() > 0)
                return 1000.0*Avg / Values_.size();
            else
                return 0;
        }
        
    private:

        std::deque<T> Values_;
        std::size_t BufferSize_{50};

};

#endif // AVG_FILTER_HPP

/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CODE_GUARD_PARAMS_HH
#define CODE_GUARD_PARAMS_HH 1

#include <chrono>
#include <atomic>

struct Params
{
    /// If this is set to true, we should abort due to a time limit.
    std::atomic<bool> * abort;

    /// The start time of the algorithm.
    std::chrono::time_point<std::chrono::steady_clock> start_time;

    bool d2graphs = false;
    bool d2cgraphs = false;
    bool induced = false;
    bool nds = false;
    bool cnds = false;
    bool degree = false;
    bool expensive_stats = false;
    bool high_wildcards = false;
    bool ilf = false;

    unsigned except = 0;
};

#endif

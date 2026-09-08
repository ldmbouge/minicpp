#pragma once

#include <chrono>

#include "Alias.hpp"

namespace gfl {
using TimePoint = std::chrono::steady_clock::time_point;
using sec = std::chrono::seconds;
using ms = std::chrono::milliseconds;
using us = std::chrono::microseconds;
using ns = std::chrono::nanoseconds;
inline TimePoint now() noexcept { return std::chrono::steady_clock::now(); }
template<typename TUnit = ns> f64 elapsed(TimePoint const& start, TimePoint const& end) noexcept {
    using namespace std::chrono;
    return duration_cast<duration<f64, typename TUnit::period>>(end - start).count();
}
template<typename TUnit = ns> f64 elapsed(TimePoint const& start) noexcept { return elapsed<TUnit>(start, now()); }
}
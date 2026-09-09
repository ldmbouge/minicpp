#pragma once

#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Alias.hpp"
#include "ChronoUtils.hpp"
#include "Macros.hpp"  // GFL_DISABLE_COPY_AND_MOVE

namespace gfl {
// A process-wide wall-clock profiler keyed by name. Each named timer keeps its
// running total and call count, so summary() can report where the time went.
// Use it through TIMED_SCOPE, which closes the timer on every exit path.
class Timer {
public:
    struct Timing {
        TimePoint begin{now()};
        f64 totalMs{0.0};
        i64 count{0};
    };

private:
    std::mutex _mutex;
    TimePoint _programBegin{now()};
    std::unordered_map<std::string, Timing> _timings;

    Timer() = default;
    // Function-local static: the C++ runtime makes the initialization
    // thread-safe, which a lazily assigned pointer would not be
    static Timer& get() {
        static Timer instance;
        return instance;
    }

public:
    GFL_DISABLE_COPY_AND_MOVE(Timer);

    // Times a scope, closing the timer however the scope is left
    class Scoped {
        std::string _name;

    public:
        explicit Scoped(std::string name) : _name(gfl::move(name)) { Timer::begin(_name); }
        ~Scoped() { Timer::end(_name); }
        GFL_DISABLE_COPY_AND_MOVE(Scoped);
    };

    static void begin(std::string const& name = "Anonymous") {
        // Read the clock before taking the lock, so waiting for it is not timed
        TimePoint const t = now();
        Timer& timer = get();
        std::lock_guard<std::mutex> const lock(timer._mutex);
        timer._timings[name].begin = t;
    }

    static void end(std::string const& name = "Anonymous", bool const printElapsed = false) {
        TimePoint const t = now();
        Timer& timer = get();
        std::lock_guard<std::mutex> const lock(timer._mutex);

        auto const it = timer._timings.find(name);
        if (it == timer._timings.end()) {
            std::cerr << "Timer::end: no timer named '" << name << "'\n";
            return;
        }

        f64 const elapsedMs = elapsed<ms>(it->second.begin, t);
        it->second.totalMs += elapsedMs;
        it->second.count += 1;
        if (printElapsed) std::cout << name << ": " << elapsedMs << " ms\n";
    }

    static void reset() {
        Timer& timer = get();
        std::lock_guard<std::mutex> const lock(timer._mutex);
        timer._timings.clear();
        timer._programBegin = now();
    }

    static std::string summary() {
        constexpr i32 NameWidth = 30;
        constexpr i32 ValueWidth = 12;

        Timer& timer = get();
        std::lock_guard<std::mutex> const lock(timer._mutex);
        f64 const total = elapsed<ms>(timer._programBegin);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(3);
        auto row = [&](std::string const& name, f64 const avg, f64 const tot, i64 const count, f64 const pct) {
            ss << "%% | " << std::left << std::setw(NameWidth) << name;
            ss << " |" << std::right << std::setw(ValueWidth) << avg << " ms";
            ss << " |" << std::right << std::setw(ValueWidth) << tot << " ms";
            ss << " |" << std::right << std::setw(ValueWidth) << count << " calls";
            ss << " |" << std::right << std::setw(ValueWidth) << pct << " %\n";
        };

        ss << "%% +-------------------\n";
        ss << "%% | Profiling Summary\n";
        ss << "%% +-------------------\n";
        ss << "%% | " << std::left << std::setw(NameWidth) << "Name";
        ss << " |" << std::right << std::setw(ValueWidth + 3) << "Average";
        ss << " |" << std::right << std::setw(ValueWidth + 3) << "Total";
        ss << " |" << std::right << std::setw(ValueWidth + 6) << "Called";
        ss << " |" << std::right << std::setw(ValueWidth + 2) << "Percentage" << "\n";

        row("Total Run Time", total, total, 1, 100.0);
        for (auto const& [name, t] : timer._timings) {
            f64 const avg = t.count > 0 ? t.totalMs / scast<f64>(t.count) : 0.0;
            f64 const pct = total > 0.0 ? 100.0 * t.totalMs / total : 0.0;
            row(name, avg, t.totalMs, t.count, pct);
        }
        return ss.str();
    }
};
}

// Times the current scope, keyed by the enclosing function's name.
// Usage: void myFunction() { GFL_TIMED_SCOPE(); ... }
#define GFL_TIMED_SCOPE() gfl::Timer::Scoped const _gflScopedTimer(__func__)

// Times the current scope, keyed by a name of your own.
// Usage: void myFunction() { GFL_TIMED_SCOPE_NAMED("inner loop"); ... }
#define GFL_TIMED_SCOPE_NAMED(name) gfl::Timer::Scoped const _gflScopedTimer(name)

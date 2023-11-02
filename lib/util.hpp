#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>

template <typename T> T square(const T t) { return t * t; }


class Logger {
public:
    explicit Logger(int log_level, const bool newline=true) :
            _log_level(log_level),
            _newline(newline),
            _oss() { }
    template <typename T>
    Logger& operator<< (const T& output) {
        if (_log_level <= LOG_LEVEL) {
            _oss << output << ' ';
        }
        return *this;
    }

    template <typename T>
    Logger& operator<< (const T* output) {
        if (_log_level <= LOG_LEVEL) {
            _oss << output << ' ';
        }
        return *this;
    }


    Logger& operator<< (decltype(std::left)& output) {
        if (_log_level <= LOG_LEVEL) {
            _oss << output << ' ';
        }
        return *this;
    }

    ~Logger() {
        if (_log_level > LOG_LEVEL) return;
        std::cout << _oss.str();
        if (_newline) {
            std::cout << std::endl;
        } else {
            std::cout << ' ';
        }
    }

    static int LOG_LEVEL;

private:
    int _log_level;
    bool _newline;
    std::ostringstream _oss;
};

#define VLOG(X) Logger(X, true)


using Duration = std::chrono::duration<double>;
using Timepoint = decltype(std::chrono::high_resolution_clock::now());

struct Timer {
    static Timer& GlobalTimer() {
        static Timer GLOBAL_TIMER;
        return GLOBAL_TIMER;
    }
    bool running = false;
    Timepoint start;
    Duration total_duration;

    Timepoint Start() {
        if (running) throw std::runtime_error("Called Start() but timer is already running");
        start = std::chrono::high_resolution_clock::now();
        running = true;
        return start;
    }

    Duration Stop() {
        if (!running) throw std::runtime_error("Timer not running but called Stop()");
        auto finish = std::chrono::high_resolution_clock::now();
        Duration duration = finish - start;
        total_duration += duration;
        running = false;
        return duration;
    }

    Duration Restart() {
        if (!running) throw std::runtime_error("Timer not running but called Restart()");
        auto finish = std::chrono::high_resolution_clock::now();
        Duration duration = finish - start;
        total_duration += duration;
        start = finish;
        return duration;
    }

};

enum Timing {
    ProposeCut,
    FlowMatch,
    Match,
    Misc,
    FlowTrim,
    ConnectedComponents,
    CutHeuristics,
    LAST_TIMING
};

static std::vector<std::string> TimingNames = {
        "ProposeCut", "FlowMatch", "MatchDFS", "Miscellaneous", "FlowTrim", "Components", "CutHeuristics"
};

struct Timings {
    static Timings& GlobalTimings() {
        static Timings GLOBAL_TIMINGS;
        return GLOBAL_TIMINGS;
    }

    Timings() {
        size_t num_entries = LAST_TIMING;
        durations.resize(num_entries, Duration(0.0));
    }
    std::vector<Duration> durations;

    void AddTiming(Timing timing, Duration duration) {
        int index = timing;
        durations[index] += duration;
    }

    void Print() {
        Duration total = Duration(0.0);
        for (const auto& dur : durations) total += dur;
        std::cout << "Category\t\ttime[s]\t\tpercentage of total runtime" << std::endl;
        for (int i = 0; i < LAST_TIMING; ++i) {
            std::cout << TimingNames[i] << "\t\t" << std::setprecision(3) << durations[i] << "\t\t" << 100.0 * durations[i] / total  << "%" << std::endl;
        }
        std::cout << "--- Total measured time " << total << " ---" << std::endl;
    }
};

// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace Clock {

// One lap of a stopwatch: its time and its share since the lap before.
struct Lap
{
    // From one.
    int index = 0;
    std::chrono::milliseconds total{};
    std::chrono::milliseconds split{};
    // Set on the shortest and the longest split once there are two laps.
    bool best = false;
    bool worst = false;

    bool operator==(const Lap&) const = default;
};

// A stopwatch: starts at zero, keeps time over pauses and records laps. The
// clock is a parameter so tests can move it themselves.
class Stopwatch
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using NowFunction = std::function<TimePoint()>;

    explicit Stopwatch(NowFunction now = {});

    void start();
    void pause();
    void toggle();
    void reset();

    bool isRunning() const { return m_running; }
    // Started and not reset since.
    bool isStarted() const { return m_running || m_accumulated > Clock::duration::zero(); }

    std::chrono::milliseconds elapsed() const;
    // The time since the last lap.
    std::chrono::milliseconds split() const;

    // Adds the time so far as a lap; false when the stopwatch was reset.
    bool addLap();
    void clearLaps();
    const std::vector<Lap>& laps() const { return m_laps; }

private:
    TimePoint now() const { return m_now ? m_now() : Clock::now(); }
    Clock::duration currentElapsed() const;
    void markBestAndWorst();

    NowFunction m_now;
    bool m_running = false;
    TimePoint m_start;
    Clock::duration m_accumulated{};
    Clock::duration m_lastLapTotal{};
    std::vector<Lap> m_laps;
};

} // namespace Clock

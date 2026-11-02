// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <functional>

namespace Clock {

enum class TimerState {
    // Set and waiting to be started, or reset.
    Idle,
    Running,
    Paused,
    // Ran out; start() begins the duration again.
    Finished,
};

// A countdown timer. The clock is a parameter so tests can move it themselves;
// update() notices when the time ran out and calls onFinished once.
class CountdownTimer
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using NowFunction = std::function<TimePoint()>;

    explicit CountdownTimer(NowFunction now = {});

    // Sets what the next start runs for; the timer goes back to idle.
    void setDuration(std::chrono::milliseconds duration);
    std::chrono::milliseconds duration() const;
    std::chrono::milliseconds remaining() const;
    // 0 right after a start, 1 when it runs out.
    double progress() const;

    void start();
    void pause();
    void toggle();
    void reset();
    // True once, on the call that finds the countdown finished.
    bool update();

    TimerState state() const { return m_state; }
    bool isRunning() const { return m_state == TimerState::Running; }

    std::function<void()> onFinished;

private:
    TimePoint now() const { return m_now ? m_now() : Clock::now(); }
    Clock::duration remainingAt(const TimePoint& time) const;

    NowFunction m_now;
    TimerState m_state = TimerState::Idle;
    Clock::duration m_duration{};
    Clock::duration m_remaining{};
    TimePoint m_start{};
};

} // namespace Clock

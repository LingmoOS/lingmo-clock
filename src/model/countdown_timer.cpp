// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model/countdown_timer.h"

#include <algorithm>

namespace Clock {

using std::chrono::duration_cast;
using std::chrono::milliseconds;

CountdownTimer::CountdownTimer(NowFunction now)
    : m_now(std::move(now))
{
}

void CountdownTimer::setDuration(std::chrono::milliseconds duration)
{
    m_duration = duration > milliseconds(0) ? duration_cast<Clock::duration>(duration)
                                            : Clock::duration::zero();
    m_remaining = m_duration;
    m_state = TimerState::Idle;
}

std::chrono::milliseconds CountdownTimer::duration() const
{
    return duration_cast<milliseconds>(m_duration);
}

CountdownTimer::Clock::duration CountdownTimer::remainingAt(const TimePoint& time) const
{
    if (m_state != TimerState::Running)
        return m_remaining;
    const Clock::duration spent = time - m_start;
    const Clock::duration remaining = m_remaining - spent;
    return remaining > Clock::duration::zero() ? remaining : Clock::duration::zero();
}

std::chrono::milliseconds CountdownTimer::remaining() const
{
    return duration_cast<milliseconds>(remainingAt(now()));
}

double CountdownTimer::progress() const
{
    if (m_duration <= Clock::duration::zero())
        return 1.0;
    const Clock::duration remaining = remainingAt(now());
    const double done = 1.0 - double(remaining.count()) / double(m_duration.count());
    return std::clamp(done, 0.0, 1.0);
}

void CountdownTimer::start()
{
    if (m_state == TimerState::Running)
        return;
    if (m_state == TimerState::Idle || m_state == TimerState::Finished || m_remaining <= Clock::duration::zero())
        m_remaining = m_duration;
    if (m_remaining <= Clock::duration::zero())
        return;
    m_start = now();
    m_state = TimerState::Running;
}

void CountdownTimer::pause()
{
    if (m_state != TimerState::Running)
        return;
    m_remaining = remainingAt(now());
    m_state = TimerState::Paused;
}

void CountdownTimer::toggle()
{
    if (m_state == TimerState::Running)
        pause();
    else
        start();
}

void CountdownTimer::reset()
{
    m_state = TimerState::Idle;
    m_remaining = m_duration;
}

bool CountdownTimer::update()
{
    if (m_state != TimerState::Running)
        return false;
    if (remainingAt(now()) > Clock::duration::zero())
        return false;
    m_remaining = Clock::duration::zero();
    m_state = TimerState::Finished;
    // Copied first: the callback may rebuild what owns the timer.
    const std::function<void()> finished = onFinished;
    if (finished)
        finished();
    return true;
}

} // namespace Clock

// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model/stopwatch.h"

#include <algorithm>

namespace Clock {

using std::chrono::duration_cast;
using std::chrono::milliseconds;

Stopwatch::Stopwatch(NowFunction now)
    : m_now(std::move(now))
{
}

Stopwatch::Clock::duration Stopwatch::currentElapsed() const
{
    Clock::duration elapsed = m_accumulated;
    if (m_running)
        elapsed += now() - m_start;
    return elapsed;
}

void Stopwatch::start()
{
    if (m_running)
        return;
    m_start = now();
    m_running = true;
}

void Stopwatch::pause()
{
    if (!m_running)
        return;
    m_accumulated += now() - m_start;
    m_running = false;
}

void Stopwatch::toggle()
{
    if (m_running)
        pause();
    else
        start();
}

void Stopwatch::reset()
{
    m_running = false;
    m_accumulated = Clock::duration::zero();
    m_lastLapTotal = Clock::duration::zero();
    m_laps.clear();
}

std::chrono::milliseconds Stopwatch::elapsed() const
{
    return duration_cast<milliseconds>(currentElapsed());
}

std::chrono::milliseconds Stopwatch::split() const
{
    const auto since = currentElapsed() - m_lastLapTotal;
    return duration_cast<milliseconds>(since > Clock::duration::zero() ? since : Clock::duration::zero());
}

bool Stopwatch::addLap()
{
    const Clock::duration total = currentElapsed();
    if (total <= Clock::duration::zero() || m_laps.size() >= size_t(999))
        return false;
    Lap lap;
    lap.index = int(m_laps.size()) + 1;
    lap.total = duration_cast<milliseconds>(total);
    lap.split = duration_cast<milliseconds>(total - m_lastLapTotal);
    m_laps.push_back(lap);
    m_lastLapTotal = total;
    markBestAndWorst();
    return true;
}

void Stopwatch::clearLaps()
{
    m_laps.clear();
    m_lastLapTotal = Clock::duration::zero();
}

void Stopwatch::markBestAndWorst()
{
    if (m_laps.size() < 2) {
        for (Lap& lap : m_laps) {
            lap.best = false;
            lap.worst = false;
        }
        return;
    }
    auto shortest = std::min_element(m_laps.begin(), m_laps.end(),
                                     [](const Lap& a, const Lap& b) { return a.split < b.split; });
    auto longest = std::max_element(m_laps.begin(), m_laps.end(),
                                    [](const Lap& a, const Lap& b) { return a.split < b.split; });
    const bool varied = shortest->split != longest->split;
    for (Lap& lap : m_laps) {
        lap.best = varied && lap.split == shortest->split;
        lap.worst = varied && lap.split == longest->split;
    }
}

} // namespace Clock

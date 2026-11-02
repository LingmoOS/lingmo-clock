// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model/timeformat.h"

#include "base/i18n.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace Clock {

using namespace std::chrono_literals;

static const char* weekdayName(int weekday)
{
    switch (weekday) {
    case 0: return Lingmo::i18n::tr("Sunday");
    case 1: return Lingmo::i18n::tr("Monday");
    case 2: return Lingmo::i18n::tr("Tuesday");
    case 3: return Lingmo::i18n::tr("Wednesday");
    case 4: return Lingmo::i18n::tr("Thursday");
    case 5: return Lingmo::i18n::tr("Friday");
    case 6: return Lingmo::i18n::tr("Saturday");
    }
    return "";
}

static const char* monthName(int month)
{
    switch (month) {
    case 1: return Lingmo::i18n::tr("January");
    case 2: return Lingmo::i18n::tr("February");
    case 3: return Lingmo::i18n::tr("March");
    case 4: return Lingmo::i18n::tr("April");
    case 5: return Lingmo::i18n::tr("May");
    case 6: return Lingmo::i18n::tr("June");
    case 7: return Lingmo::i18n::tr("July");
    case 8: return Lingmo::i18n::tr("August");
    case 9: return Lingmo::i18n::tr("September");
    case 10: return Lingmo::i18n::tr("October");
    case 11: return Lingmo::i18n::tr("November");
    case 12: return Lingmo::i18n::tr("December");
    }
    return "";
}

static std::string formatted(const char* pattern, ...)
{
    char buffer[64];
    va_list arguments;
    va_start(arguments, pattern);
    std::vsnprintf(buffer, sizeof(buffer), pattern, arguments);
    va_end(arguments);
    return buffer;
}

std::string formatClockTime(const TimeDigits& time, bool twentyFourHour)
{
    const int hour = ((time.hour % 24) + 24) % 24;
    const int minute = std::abs(time.minute) % 60;
    const int second = std::abs(time.second) % 60;
    if (twentyFourHour)
        return formatted("%02d:%02d:%02d", hour, minute, second);
    const int twelve = hour % 12 == 0 ? 12 : hour % 12;
    return formatted("%d:%02d:%02d %s", twelve, minute, second,
                     hour < 12 ? Lingmo::i18n::tr("AM") : Lingmo::i18n::tr("PM"));
}

ClockHands clockHands(int hour, int minute, int second, double secondFraction)
{
    const double seconds = double(second) + secondFraction;
    ClockHands hands;
    // Each hand carries the finer one on by its share of its own turn.
    hands.second = seconds / 60.0 * 360.0;
    hands.minute = (double(minute) + seconds / 60.0) / 60.0 * 360.0;
    hands.hour = (double(((hour % 12) + 12) % 12) + double(minute) / 60.0 + seconds / 3600.0) / 12.0
        * 360.0;
    return hands;
}

std::string formatStopwatch(std::chrono::milliseconds elapsed)
{
    using std::chrono::duration_cast;
    if (elapsed < 0ms)
        elapsed = 0ms;
    const int64_t totalMs = elapsed.count();
    const int centiseconds = int(totalMs % 1000 / 10);
    const int64_t totalSeconds = totalMs / 1000;
    const int seconds = int(totalSeconds % 60);
    const int minutes = int(totalSeconds / 60 % 60);
    const int hours = int(totalSeconds / 3600);
    if (hours > 0)
        return formatted("%d:%02d:%02d.%02d", hours, minutes, seconds, centiseconds);
    return formatted("%02d:%02d.%02d", minutes, seconds, centiseconds);
}

std::string formatCountdown(std::chrono::milliseconds remaining)
{
    if (remaining <= 0ms)
        return "00:00";
    const int64_t totalMs = remaining.count();
    const int64_t totalSeconds = totalMs / 1000;
    if (totalMs < 10000) {
        return formatted("%02d.%d", int(totalMs / 1000), int(totalMs % 1000 / 100));
    }
    const int seconds = int(totalSeconds % 60);
    const int minutes = int(totalSeconds / 60 % 60);
    const int hours = int(totalSeconds / 3600);
    if (hours > 0)
        return formatted("%d:%02d:%02d", hours, minutes, seconds);
    return formatted("%02d:%02d", minutes, seconds);
}

std::string formatMinutesSeconds(int minutes, int seconds)
{
    if (minutes < 0)
        minutes = 0;
    if (seconds < 0)
        seconds = 0;
    return formatted("%02d:%02d", minutes, seconds);
}

std::string formatUtcOffset(int offsetSeconds)
{
    const char sign = offsetSeconds < 0 ? '-' : '+';
    const int total = std::abs(offsetSeconds);
    return formatted("UTC%c%02d:%02d", sign, total / 3600, total % 3600 / 60);
}

std::string formatLongDate(const std::tm& local)
{
    return Lingmo::i18n::format(Lingmo::i18n::tr("{weekday}, {day} {month} {year}"),
                                {{"weekday", weekdayName(local.tm_wday)},
                                 {"month", monthName(local.tm_mon + 1)},
                                 {"day", std::to_string(local.tm_mday)},
                                 {"year", std::to_string(local.tm_year + 1900)}});
}

} // namespace Clock

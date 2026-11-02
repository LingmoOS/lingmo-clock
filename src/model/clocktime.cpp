// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model/clocktime.h"

#include <cstring>
#include <langinfo.h>

namespace Clock {

ClockReading readingFrom(const std::tm& local, std::chrono::nanoseconds subsecond)
{
    ClockReading reading;
    reading.year = local.tm_year + 1900;
    reading.month = local.tm_mon + 1;
    reading.day = local.tm_mday;
    reading.weekday = local.tm_wday;
    reading.hour = local.tm_hour;
    reading.minute = local.tm_min;
    reading.second = local.tm_sec;
    reading.nanosecond = subsecond;
    return reading;
}

ClockReading currentReading(std::chrono::system_clock::time_point now)
{
    const std::chrono::system_clock::duration sinceEpoch = now.time_since_epoch();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    const auto wholeSeconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
    const std::chrono::nanoseconds subsecond =
        std::chrono::duration_cast<std::chrono::nanoseconds>(sinceEpoch - wholeSeconds);
    std::tm local{};
    localtime_r(&seconds, &local);
    return readingFrom(local, subsecond);
}

TimeDigits digitsOf(const ClockReading& reading)
{
    return {reading.hour, reading.minute, reading.second};
}

bool localeUsesTwentyFourHourClock()
{
    const char* format = nl_langinfo(T_FMT);
    return format == nullptr || std::strstr(format, "%p") == nullptr
        || std::strstr(format, "%r") != nullptr;
}

} // namespace Clock

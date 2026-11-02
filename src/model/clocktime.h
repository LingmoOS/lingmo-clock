// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <ctime>

#include "model/timeformat.h"

namespace Clock {

// A local date and time, with the part of its second already run.
struct ClockReading
{
    int year = 1970;
    int month = 1;
    int day = 1;
    // 0 is Sunday.
    int weekday = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    std::chrono::nanoseconds nanosecond{};

    bool operator==(const ClockReading&) const = default;
};

ClockReading readingFrom(const std::tm& local, std::chrono::nanoseconds subsecond = {});
ClockReading currentReading(std::chrono::system_clock::time_point now =
                                std::chrono::system_clock::now());

TimeDigits digitsOf(const ClockReading& reading);

// Whether the session's locale writes a twelve-hour time, as the digital line
// follows it.
bool localeUsesTwentyFourHourClock();

} // namespace Clock

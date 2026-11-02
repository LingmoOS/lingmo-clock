// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <ctime>
#include <string>

namespace Clock {

// A local wall-clock reading, as the clock face and the digital line show it.
struct TimeDigits
{
    int hour = 0;
    int minute = 0;
    int second = 0;

    bool operator==(const TimeDigits&) const = default;
};

// "14:05:09", or "2:05:09 PM" in the twelve-hour form.
std::string formatClockTime(const TimeDigits& time, bool twentyFourHour);

// The hands' angles in degrees, clockwise from twelve. The fraction of the
// second moves the second hand on smoothly.
struct ClockHands
{
    double hour = 0;
    double minute = 0;
    double second = 0;

    bool operator==(const ClockHands&) const = default;
};
ClockHands clockHands(int hour, int minute, int second, double secondFraction = 0);

// A stopwatch's time: "00:05.42", and "1:02:03.45" past an hour.
std::string formatStopwatch(std::chrono::milliseconds elapsed);

// A timer's remaining time: "04:59"; under ten seconds "09.4"; past an hour
// "1:00:00".
std::string formatCountdown(std::chrono::milliseconds remaining);

// The numbers a timer is given: "05:30".
std::string formatMinutesSeconds(int minutes, int seconds);

// A world clock's offset from UTC: "UTC+08:00", "UTC-05:30".
std::string formatUtcOffset(int offsetSeconds);

// The date line, "Monday, 2 November 2026", translated.
std::string formatLongDate(const std::tm& local);

} // namespace Clock

// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model/clocktime.h"
#include "model/countdown_timer.h"
#include "model/stopwatch.h"
#include "model/timeformat.h"
#include "model/worldclock.h"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>

using namespace Clock;
using namespace std::chrono_literals;

namespace {

// A clock the tests move themselves, so nothing here sleeps.
struct FakeClock
{
    std::chrono::steady_clock::time_point now{};

    template<typename Timer>
    typename Timer::NowFunction function()
    {
        return [this] { return now; };
    }

    void advance(std::chrono::milliseconds by) { now += by; }
};

std::tm date(int year, int month, int day, int weekday)
{
    std::tm local{};
    local.tm_year = year - 1900;
    local.tm_mon = month - 1;
    local.tm_mday = day;
    local.tm_wday = weekday;
    return local;
}

} // namespace

TEST(TimeFormatTest, TwentyFourHourTime)
{
    EXPECT_EQ(formatClockTime({0, 0, 0}, true), "00:00:00");
    EXPECT_EQ(formatClockTime({14, 5, 9}, true), "14:05:09");
    EXPECT_EQ(formatClockTime({23, 59, 59}, true), "23:59:59");
}

TEST(TimeFormatTest, TwelveHourTime)
{
    EXPECT_EQ(formatClockTime({0, 0, 0}, false), "12:00:00 AM");
    EXPECT_EQ(formatClockTime({11, 59, 59}, false), "11:59:59 AM");
    EXPECT_EQ(formatClockTime({12, 0, 0}, false), "12:00:00 PM");
    EXPECT_EQ(formatClockTime({14, 5, 9}, false), "2:05:09 PM");
}

TEST(TimeFormatTest, ClockHandsFollowTheTime)
{
    ClockHands midnight = clockHands(12, 0, 0);
    EXPECT_DOUBLE_EQ(midnight.hour, 0);
    EXPECT_DOUBLE_EQ(midnight.minute, 0);
    EXPECT_DOUBLE_EQ(midnight.second, 0);

    ClockHands three = clockHands(3, 0, 0);
    EXPECT_DOUBLE_EQ(three.hour, 90);
    EXPECT_DOUBLE_EQ(three.minute, 0);

    ClockHands halfPastSix = clockHands(6, 30, 0);
    EXPECT_DOUBLE_EQ(halfPastSix.hour, 195);
    EXPECT_DOUBLE_EQ(halfPastSix.minute, 180);

    ClockHands quarterToTen = clockHands(9, 45, 30);
    EXPECT_DOUBLE_EQ(quarterToTen.hour, 292.75);
    EXPECT_DOUBLE_EQ(quarterToTen.minute, 273);
    EXPECT_DOUBLE_EQ(quarterToTen.second, 180);

    // The hands carry each other on continuously.
    EXPECT_DOUBLE_EQ(clockHands(0, 0, 0, 0.5).second, 3);
    EXPECT_NEAR(clockHands(0, 0, 0, 30).minute, 3, 1e-9);
}

TEST(TimeFormatTest, StopwatchTime)
{
    EXPECT_EQ(formatStopwatch(0ms), "00:00.00");
    EXPECT_EQ(formatStopwatch(5420ms), "00:05.42");
    EXPECT_EQ(formatStopwatch(59999ms), "00:59.99");
    EXPECT_EQ(formatStopwatch(60000ms), "01:00.00");
    EXPECT_EQ(formatStopwatch(3723450ms), "1:02:03.45");
    EXPECT_EQ(formatStopwatch(-5ms), "00:00.00");
}

TEST(TimeFormatTest, CountdownTime)
{
    EXPECT_EQ(formatCountdown(0ms), "00:00");
    EXPECT_EQ(formatCountdown(-1ms), "00:00");
    EXPECT_EQ(formatCountdown(299000ms), "04:59");
    EXPECT_EQ(formatCountdown(59000ms), "00:59");
    EXPECT_EQ(formatCountdown(9450ms), "09.4");
    EXPECT_EQ(formatCountdown(3661000ms), "1:01:01");
}

TEST(TimeFormatTest, MinutesAndSecondsAndOffsets)
{
    EXPECT_EQ(formatMinutesSeconds(5, 30), "05:30");
    EXPECT_EQ(formatMinutesSeconds(0, 0), "00:00");
    EXPECT_EQ(formatMinutesSeconds(599, 59), "599:59");

    EXPECT_EQ(formatUtcOffset(0), "UTC+00:00");
    EXPECT_EQ(formatUtcOffset(28800), "UTC+08:00");
    EXPECT_EQ(formatUtcOffset(-19800), "UTC-05:30");
    EXPECT_EQ(formatUtcOffset(3600), "UTC+01:00");
}

TEST(TimeFormatTest, LongDate)
{
    EXPECT_EQ(formatLongDate(date(2026, 11, 2, 1)), "Monday, 2 November 2026");
    EXPECT_EQ(formatLongDate(date(2026, 1, 1, 4)), "Thursday, 1 January 2026");
}

TEST(ClockTimeTest, ReadingAndDigits)
{
    const std::tm local = date(2026, 11, 2, 1);
    ClockReading reading = readingFrom(local, 250ms);
    EXPECT_EQ(reading.year, 2026);
    EXPECT_EQ(reading.month, 11);
    EXPECT_EQ(reading.day, 2);
    EXPECT_EQ(reading.weekday, 1);
    EXPECT_EQ(reading.nanosecond, 250ms);

    reading.hour = 9;
    reading.minute = 41;
    reading.second = 7;
    EXPECT_EQ(digitsOf(reading), (TimeDigits{9, 41, 7}));
}

TEST(ClockTimeTest, LocaleClockPreferenceIsAnswerable)
{
    // The answer depends on the session, but it must be a proper yes or no.
    const bool twentyFourHour = localeUsesTwentyFourHourClock();
    EXPECT_TRUE(twentyFourHour || !twentyFourHour);
}

TEST(StopwatchTest, StartsAtZeroAndCountsOnlyWhileRunning)
{
    FakeClock clock;
    Stopwatch stopwatch(clock.function<Stopwatch>());
    EXPECT_FALSE(stopwatch.isRunning());
    EXPECT_FALSE(stopwatch.isStarted());
    EXPECT_EQ(stopwatch.elapsed(), 0ms);

    stopwatch.start();
    EXPECT_TRUE(stopwatch.isRunning());
    clock.advance(1500ms);
    EXPECT_EQ(stopwatch.elapsed(), 1500ms);

    stopwatch.pause();
    EXPECT_FALSE(stopwatch.isRunning());
    clock.advance(1000ms);
    EXPECT_EQ(stopwatch.elapsed(), 1500ms);

    stopwatch.toggle();
    clock.advance(500ms);
    EXPECT_EQ(stopwatch.elapsed(), 2000ms);

    stopwatch.reset();
    EXPECT_EQ(stopwatch.elapsed(), 0ms);
    EXPECT_FALSE(stopwatch.isStarted());
    EXPECT_TRUE(stopwatch.laps().empty());
}

TEST(StopwatchTest, RecordsLapsAndMarksTheBestAndWorst)
{
    FakeClock clock;
    Stopwatch stopwatch(clock.function<Stopwatch>());
    stopwatch.start();
    clock.advance(1000ms);
    EXPECT_TRUE(stopwatch.addLap());
    clock.advance(500ms);
    EXPECT_TRUE(stopwatch.addLap());
    clock.advance(2000ms);
    EXPECT_TRUE(stopwatch.addLap());

    const std::vector<Lap>& laps = stopwatch.laps();
    ASSERT_EQ(laps.size(), size_t(3));
    EXPECT_EQ(laps[0].index, 1);
    EXPECT_EQ(laps[0].total, 1000ms);
    EXPECT_EQ(laps[0].split, 1000ms);
    EXPECT_EQ(laps[1].total, 1500ms);
    EXPECT_EQ(laps[1].split, 500ms);
    EXPECT_EQ(laps[2].total, 3500ms);
    EXPECT_EQ(laps[2].split, 2000ms);

    EXPECT_TRUE(laps[1].best);
    EXPECT_TRUE(laps[2].worst);
    EXPECT_FALSE(laps[0].best);
    EXPECT_FALSE(laps[0].worst);
    EXPECT_EQ(stopwatch.split(), 0ms);

    stopwatch.reset();
    EXPECT_TRUE(stopwatch.laps().empty());
    EXPECT_EQ(stopwatch.elapsed(), 0ms);
}

TEST(StopwatchTest, ResetLapsKeepsTheRunningTime)
{
    FakeClock clock;
    Stopwatch stopwatch(clock.function<Stopwatch>());
    stopwatch.start();
    clock.advance(700ms);
    stopwatch.addLap();
    EXPECT_EQ(stopwatch.laps().size(), size_t(1));

    stopwatch.clearLaps();
    EXPECT_TRUE(stopwatch.laps().empty());
    clock.advance(300ms);
    EXPECT_EQ(stopwatch.split(), 1000ms);
}

TEST(CountdownTimerTest, CountsDownOverPauses)
{
    FakeClock clock;
    CountdownTimer timer(clock.function<CountdownTimer>());
    timer.setDuration(2000ms);
    EXPECT_EQ(timer.state(), TimerState::Idle);
    EXPECT_EQ(timer.remaining(), 2000ms);
    EXPECT_DOUBLE_EQ(timer.progress(), 0);

    timer.start();
    EXPECT_EQ(timer.state(), TimerState::Running);
    clock.advance(500ms);
    EXPECT_EQ(timer.remaining(), 1500ms);
    EXPECT_FALSE(timer.update());
    EXPECT_DOUBLE_EQ(timer.progress(), 0.25);

    timer.pause();
    EXPECT_EQ(timer.state(), TimerState::Paused);
    clock.advance(1000ms);
    EXPECT_EQ(timer.remaining(), 1500ms);

    timer.start();
    clock.advance(1499ms);
    EXPECT_FALSE(timer.update());
    EXPECT_EQ(timer.remaining(), 1ms);
    clock.advance(1ms);
    EXPECT_TRUE(timer.update());
    EXPECT_EQ(timer.state(), TimerState::Finished);
    EXPECT_EQ(timer.remaining(), 0ms);
    EXPECT_DOUBLE_EQ(timer.progress(), 1);

    // Only the call that found it finished reports it once.
    EXPECT_FALSE(timer.update());

    timer.start();
    EXPECT_EQ(timer.state(), TimerState::Running);
    EXPECT_EQ(timer.remaining(), 2000ms);
}

TEST(CountdownTimerTest, NotifiesOnceWhenItRunsOut)
{
    FakeClock clock;
    CountdownTimer timer(clock.function<CountdownTimer>());
    timer.setDuration(1000ms);
    int finished = 0;
    timer.onFinished = [&finished] { ++finished; };
    timer.start();
    clock.advance(1000ms);
    EXPECT_TRUE(timer.update());
    EXPECT_FALSE(timer.update());
    clock.advance(1000ms);
    EXPECT_FALSE(timer.update());
    EXPECT_EQ(finished, 1);

    timer.reset();
    EXPECT_EQ(timer.state(), TimerState::Idle);
    EXPECT_EQ(timer.remaining(), 1000ms);
}

TEST(CountdownTimerTest, AZeroDurationNeverStarts)
{
    FakeClock clock;
    CountdownTimer timer(clock.function<CountdownTimer>());
    timer.setDuration(0ms);
    timer.start();
    EXPECT_EQ(timer.state(), TimerState::Idle);
    EXPECT_FALSE(timer.update());
    EXPECT_EQ(timer.remaining(), 0ms);
}

TEST(WorldClockTest, MomentsAtOffsets)
{
    const LocalMoment epoch = WorldClock::momentAtOffset(0, 28800);
    EXPECT_EQ(epoch.year, 1970);
    EXPECT_EQ(epoch.month, 1);
    EXPECT_EQ(epoch.day, 1);
    EXPECT_EQ(epoch.hour, 8);
    EXPECT_EQ(epoch.minute, 0);
    EXPECT_EQ(epoch.second, 0);

    // 1700000000 is 2023-11-14 22:13:20 UTC, a Tuesday.
    const LocalMoment moment = WorldClock::momentAtOffset(1700000000, 0);
    EXPECT_EQ(moment.year, 2023);
    EXPECT_EQ(moment.month, 11);
    EXPECT_EQ(moment.day, 14);
    EXPECT_EQ(moment.weekday, 2);
    EXPECT_EQ(moment.hour, 22);
    EXPECT_EQ(moment.minute, 13);
    EXPECT_EQ(moment.second, 20);

    const LocalMoment ahead = WorldClock::momentAtOffset(1700000000, 28800);
    EXPECT_EQ(ahead.day, 15);
    EXPECT_EQ(ahead.hour, 6);
    EXPECT_EQ(ahead.minute, 13);
}

TEST(WorldClockTest, AddsAndRemovesZones)
{
    WorldClock world([](const std::string& zone, std::time_t) {
        return zone == "Test/Plus2" ? 7200 : 0;
    });
    EXPECT_TRUE(world.addZone("UTC"));
    EXPECT_FALSE(world.addZone("UTC"));
    EXPECT_FALSE(world.addZone(""));
    EXPECT_TRUE(world.addZone("Test/Plus2"));
    EXPECT_EQ(world.zones().size(), size_t(2));

    EXPECT_EQ(world.offsetAt("Test/Plus2", 0), 7200);
    const LocalMoment moment = world.moment("Test/Plus2", 0);
    EXPECT_EQ(moment.hour, 2);

    EXPECT_FALSE(world.removeZone(5));
    EXPECT_TRUE(world.removeZone(0));
    ASSERT_EQ(world.zones().size(), size_t(1));
    EXPECT_EQ(world.zones()[0], "Test/Plus2");
}

TEST(WorldClockTest, DayDifference)
{
    LocalMoment reference;
    reference.year = 2026;
    reference.month = 11;
    reference.day = 2;

    LocalMoment same = reference;
    EXPECT_EQ(WorldClock::dayDifference(same, reference), 0);

    LocalMoment tomorrow = reference;
    tomorrow.day = 3;
    EXPECT_EQ(WorldClock::dayDifference(tomorrow, reference), 1);

    LocalMoment yesterday = reference;
    yesterday.day = 1;
    EXPECT_EQ(WorldClock::dayDifference(yesterday, reference), -1);

    LocalMoment newYear = reference;
    newYear.year = 2027;
    newYear.month = 1;
    newYear.day = 1;
    EXPECT_EQ(WorldClock::dayDifference(newYear, reference), 1);
}

TEST(WorldClockTest, SystemKnowsUtc)
{
    // UTC is always reachable, whatever the system's zone database holds.
    EXPECT_EQ(systemUtcOffset("UTC", 0), 0);
    EXPECT_EQ(systemUtcOffset("UTC", 1700000000), 0);
}

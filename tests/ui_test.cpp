// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clockwindow.h"

#include "views/alarmpage.h"
#include "views/clockpage.h"
#include "views/stopwatchpage.h"
#include "views/timerpage.h"
#include "views/worldclockpage.h"

#include "base/event_loop.h"
#include "model/clocktime.h"
#include "ui/application.h"
#include "ui/keys.h"
#include "ui/test_input.h"
#include "ui/theme.h"
#include "ui/window.h"

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <memory>
#include <poll.h>

using namespace Lingmo;
using namespace Lingmo::ui;
using namespace std::chrono_literals;
using Clock::ClockReading;
using Clock::ClockWindow;

namespace {

void spin(int milliseconds)
{
    base::EventLoop& loop = Application::instance()->loop();
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    while (std::chrono::steady_clock::now() < end) {
        pollfd descriptor{loop.fd(), POLLIN, 0};
        ::poll(&descriptor, 1, 10);
        loop.dispatch();
    }
}

bool waitUntil(const std::function<bool()>& done, int timeoutMilliseconds)
{
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMilliseconds);
    while (!done()) {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        pollfd descriptor{Application::instance()->loop().fd(), POLLIN, 0};
        ::poll(&descriptor, 1, 10);
        Application::instance()->loop().dispatch();
    }
    return true;
}

ClockReading fixedReading()
{
    ClockReading reading;
    reading.year = 2026;
    reading.month = 11;
    reading.day = 2;
    reading.weekday = 1;
    reading.hour = 14;
    reading.minute = 5;
    reading.second = 9;
    return reading;
}

} // namespace

class ClockWindowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Theme::setCurrent(Theme::light());
        m_window = std::make_unique<ClockWindow>();
        m_window->window().setScale(1);
        TestInput::configure(m_window->window(), 900, 680, {true});
        TestInput::setKeyboardFocus(m_window->window(), true);
        m_window->show();
        spin(30);
    }

    void TearDown() override
    {
        m_window.reset();
        spin(20);
    }

    ClockWindow& window() { return *m_window; }

    void pressKey(uint32_t key, uint32_t modifiers = NoModifier)
    {
        TestInput::typeKey(window().window(), key, modifiers);
        spin(10);
    }

private:
    std::unique_ptr<ClockWindow> m_window;
};

TEST_F(ClockWindowTest, StartsOnTheClockAndSwitchesPages)
{
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageClock));
    EXPECT_TRUE(window().clockPage()->isVisible());
    EXPECT_FALSE(window().stopwatchPage()->isVisible());
    window().window().grab();
    EXPECT_GT(window().clockPage()->geometry().width, 500);

    window().selectPage(ClockWindow::PageStopwatch);
    spin(10);
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageStopwatch));
    EXPECT_TRUE(window().stopwatchPage()->isVisible());
    EXPECT_FALSE(window().clockPage()->isVisible());

    window().selectPage(int(ClockWindow::PageCount) + 3);
    spin(10);
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageTimer));
}

TEST_F(ClockWindowTest, KeyboardShortcutsSwitchPages)
{
    pressKey(key::letter('4'), ControlModifier);
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageStopwatch));

    pressKey(key::letter('1'), ControlModifier);
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageClock));

    pressKey(key::Tab, ControlModifier);
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageAlarm));

    pressKey(key::Tab, ControlModifier | ShiftModifier);
    EXPECT_EQ(window().currentPageIndex(), int(ClockWindow::PageClock));
}

TEST_F(ClockWindowTest, ClockShowsTheDateAndTheDigitalTime)
{
    window().clockPage()->setTwentyFourHourClock(true);
    window().clockPage()->setTimeSource(fixedReading);
    spin(10);
    EXPECT_EQ(window().clockPage()->timeText(), "14:05:09");
    EXPECT_EQ(window().clockPage()->dateText(), "Monday, 2 November 2026");

    window().clockPage()->setTwentyFourHourClock(false);
    spin(10);
    EXPECT_EQ(window().clockPage()->timeText(), "2:05:09 PM");
}

TEST_F(ClockWindowTest, TogglesTheClockFormat)
{
    window().clockPage()->setTimeSource(fixedReading);
    window().clockPage()->setTwentyFourHourClock(true);
    spin(10);
    EXPECT_TRUE(window().clockPage()->usesTwentyFourHourClock());
    window().clockPage()->toggleTwentyFourHourClock();
    spin(10);
    EXPECT_FALSE(window().clockPage()->usesTwentyFourHourClock());
    EXPECT_EQ(window().clockPage()->timeText(), "2:05:09 PM");
    window().clockPage()->toggleTwentyFourHourClock();
    spin(10);
    EXPECT_EQ(window().clockPage()->timeText(), "14:05:09");
}

TEST_F(ClockWindowTest, AlarmPageKeepsItsAlarms)
{
    window().selectPage(ClockWindow::PageAlarm);
    spin(10);
    ASSERT_EQ(window().alarmPage()->alarmCount(), 1u);
    ASSERT_TRUE(window().alarmPage()->addAlarm(6, 30));
    ASSERT_EQ(window().alarmPage()->alarmCount(), 2u);
    EXPECT_EQ(window().alarmPage()->alarmHour(1), 6);
    EXPECT_EQ(window().alarmPage()->alarmMinute(1), 30);
    EXPECT_TRUE(window().alarmPage()->setAlarmEnabled(1, false));
    EXPECT_FALSE(window().alarmPage()->alarmEnabled(1));
    ASSERT_TRUE(window().alarmPage()->removeAlarm(1));
    EXPECT_EQ(window().alarmPage()->alarmCount(), 1u);
}

TEST_F(ClockWindowTest, StopwatchShortcutsStartLapPauseAndReset)
{
    window().selectPage(ClockWindow::PageStopwatch);
    spin(10);

    pressKey(key::letter('s'));
    EXPECT_TRUE(window().stopwatchPage()->isRunning());
    ASSERT_TRUE(waitUntil([this] { return window().stopwatchPage()->stopwatch().elapsed() >= 30ms; },
                          2000));

    pressKey(key::letter('l'));
    EXPECT_EQ(window().stopwatchPage()->lapCount(), size_t(1));
    EXPECT_FALSE(window().stopwatchPage()->lapSplitText(0).empty());

    pressKey(key::letter('s'));
    EXPECT_FALSE(window().stopwatchPage()->isRunning());
    const auto pausedElapsed = window().stopwatchPage()->stopwatch().elapsed();
    spin(60);
    EXPECT_EQ(window().stopwatchPage()->stopwatch().elapsed(), pausedElapsed);

    pressKey(key::letter('r'));
    EXPECT_EQ(window().stopwatchPage()->lapCount(), size_t(0));
    EXPECT_EQ(window().stopwatchPage()->timeText(), "00:00.00");
}

TEST_F(ClockWindowTest, TimerCountsDownAndReportsFinishing)
{
    window().selectPage(ClockWindow::PageTimer);
    spin(10);
    int notified = 0;
    window().timerPage()->setOnFinished([&notified] { ++notified; });

    window().timerPage()->setMinutesSeconds(0, 1);
    EXPECT_EQ(window().timerPage()->state(), Clock::TimerState::Idle);
    EXPECT_EQ(window().timerPage()->duration(), 1000ms);
    EXPECT_EQ(window().timerPage()->timeText(), "01.0");
    EXPECT_EQ(window().timerPage()->captionText(), "Ready");

    window().timerPage()->start();
    EXPECT_EQ(window().timerPage()->state(), Clock::TimerState::Running);
    ASSERT_TRUE(waitUntil(
        [this] { return window().timerPage()->state() == Clock::TimerState::Finished; }, 5000));
    EXPECT_EQ(notified, 1);
    EXPECT_EQ(window().timerPage()->timeText(), "00:00");
    EXPECT_EQ(window().timerPage()->captionText(), "Time's up!");
    EXPECT_DOUBLE_EQ(window().timerPage()->progress(), 1);

    window().timerPage()->reset();
    EXPECT_EQ(window().timerPage()->state(), Clock::TimerState::Idle);
    EXPECT_EQ(window().timerPage()->remaining(), 1000ms);
}

TEST_F(ClockWindowTest, TimerInputsAreTheDuration)
{
    window().timerPage()->setMinutesSeconds(2, 30);
    EXPECT_EQ(window().timerPage()->minutes(), 2);
    EXPECT_EQ(window().timerPage()->seconds(), 30);
    EXPECT_EQ(window().timerPage()->duration(), 150s);
    EXPECT_EQ(window().timerPage()->timeText(), "02:30");

    window().timerPage()->setMinutesSeconds(0, 0);
    EXPECT_EQ(window().timerPage()->duration(), 0ms);
    window().timerPage()->start();
    EXPECT_FALSE(window().timerPage()->state() == Clock::TimerState::Running);

    window().selectPage(ClockWindow::PageClock);
    spin(10);
}

TEST_F(ClockWindowTest, WorldClockShowsEachCityTime)
{
    window().selectPage(ClockWindow::PageWorldClock);
    spin(10);
    auto* page = window().worldClockPage();
    page->setNowSource([] { return std::time_t(1700000000); });
    EXPECT_EQ(page->cityCount(), size_t(4));

    EXPECT_TRUE(page->addCity("UTC"));
    ASSERT_EQ(page->cityCount(), size_t(5));
    EXPECT_EQ(page->cityZone(4), "UTC");
    EXPECT_EQ(page->cityTimeText(4), "22:13");
    EXPECT_EQ(page->cityCaptionText(4).substr(0, 9), "UTC+00:00");

    EXPECT_TRUE(page->removeCity(4));
    EXPECT_EQ(page->cityCount(), size_t(4));
    EXPECT_FALSE(page->removeCity(9));
}

TEST_F(ClockWindowTest, GrabsTheWindowAtItsSize)
{
    const Image image = window().window().grab();
    EXPECT_FALSE(image.isNull());
    EXPECT_EQ(image.width(), 900);
    EXPECT_EQ(image.height(), 680);
}

TEST_F(ClockWindowTest, DarkThemeStillGrabs)
{
    Theme::setCurrent(Theme::dark());
    spin(20);
    const Image image = window().window().grab();
    EXPECT_FALSE(image.isNull());
    // The theme's dark background is behind the content.
    EXPECT_LT(image.pixel(4, 4).r, 60);
    Theme::setCurrent(Theme::light());
    spin(10);
}

// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>

#include "base/signal.h"
#include "ui/window.h"

namespace Lingmo::ui {
class Label;
class SegmentedControl;
class Toast;
} // namespace Lingmo::ui

namespace Clock {

class AlarmPage;
class ClockPage;
class StopwatchPage;
class TimerPage;
class WorldClockPage;

// The clock's window: the header, the four pages' switch and the pages
// themselves, with the shortcuts that reach them.
class ClockWindow
{
public:
    enum Page {
        PageClock = 0,
        PageAlarm,
        PageWorldClock,
        PageStopwatch,
        PageTimer,
        PageCount,
    };

    ClockWindow();
    ~ClockWindow();

    ClockWindow(const ClockWindow&) = delete;
    ClockWindow& operator=(const ClockWindow&) = delete;

    Lingmo::ui::Window& window() { return m_window; }
    void show();

    void selectPage(int index);
    int currentPageIndex() const { return m_currentPage; }
    Lingmo::ui::SegmentedControl* pageSwitch() const;

    ClockPage* clockPage() const;
    AlarmPage* alarmPage() const;
    WorldClockPage* worldClockPage() const;
    StopwatchPage* stopwatchPage() const;
    TimerPage* timerPage() const;

    void showNotification(const std::string& text);
    void retranslate();

private:
    class Root;
    class HeaderContent;

    void bindShortcuts();
    void updateActivePage();

    Lingmo::ui::Window m_window;
    Root* m_root = nullptr;
    std::unique_ptr<Lingmo::ui::Toast> m_toast;
    Lingmo::base::Subscription m_languageChanged;
    int m_currentPage = PageClock;
};

} // namespace Clock

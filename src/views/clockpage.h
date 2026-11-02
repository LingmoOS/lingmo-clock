// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>

#include "base/timer.h"
#include "model/clocktime.h"
#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Label;
} // namespace Lingmo::ui

namespace Clock {

class AnalogClock;
class PillButton;

// The clock's own page: the big analog face over the digital time with its
// seconds and the date.
class ClockPage : public Lingmo::ui::View
{
public:
    ClockPage();
    ~ClockPage() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }

    // For tests and captures: reads the time from here instead of the system.
    void setTimeSource(std::function<ClockReading()> source);

    AnalogClock* analogClock() const { return m_clock; }
    std::string timeText() const;
    std::string dateText() const;

    // Follows the session locale.
    bool usesTwentyFourHourClock() const { return m_twentyFourHour; }
    void setTwentyFourHourClock(bool twentyFourHour);
    void toggleTwentyFourHourClock();

    void retranslate();
    void update();

    Lingmo::ui::SizeF sizeHint() const override;

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    std::string dateTextFor(const ClockReading& reading) const;

    AnalogClock* m_clock;
    Lingmo::ui::Label* m_time;
    Lingmo::ui::Label* m_date;
    PillButton* m_formatButton;
    Lingmo::base::Timer m_tick;
    std::function<ClockReading()> m_timeSource;
    ClockReading m_reading;
    bool m_active = false;
    bool m_twentyFourHour = true;
    // What the labels showed last, so a tick in between changes nothing.
    int m_shownSecond = -1;
    int m_shownDay = -1;
};

} // namespace Clock

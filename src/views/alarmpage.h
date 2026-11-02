// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "base/settings.h"
#include "base/timer.h"
#include "model/clocktime.h"
#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Column;
class Label;
class ScrollView;
}

namespace Clock {

class NumberStepper;
class PillButton;

// The alarm page: a daily alarm can be added for any time, switched on or off
// and removed. The page owns the alarm clock, so it keeps running while the
// window is open and reports when one goes off.
class AlarmPage : public Lingmo::ui::View
{
public:
    AlarmPage();
    ~AlarmPage() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }

    bool addAlarm(int hour, int minute);
    bool removeAlarm(size_t index);
    bool setAlarmEnabled(size_t index, bool enabled);
    size_t alarmCount() const { return m_alarms.size(); }
    int alarmHour(size_t index) const;
    int alarmMinute(size_t index) const;
    bool alarmEnabled(size_t index) const;

    // For tests: the page reads the clock from here.
    void setNowSource(std::function<ClockReading()> source);
    void setOnAlarm(std::function<void(const std::string&)> callback) {
        m_onAlarm = std::move(callback);
    }

    void retranslate();
    Lingmo::ui::SizeF sizeHint() const override;

    // Checks the alarms against the current time; called by the page's timer.
    void checkAlarms();

    // Saves all alarms to ~/.config/lingmo/clock.conf and loads them back.
    void saveAlarms();
    void loadAlarms();

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    class AlarmRow;

    struct Alarm
    {
        int hour = 7;
        int minute = 0;
        bool enabled = true;
        bool fired = false;
    };

    static std::string formatAlarm(int hour, int minute);
    void rebuildRows();
    void addFromInputs();
    void updateEmptyState();

    std::vector<Alarm> m_alarms;
    std::vector<AlarmRow*> m_rows;
    Lingmo::ui::ScrollView* m_scroll = nullptr;
    Lingmo::ui::Column* m_list = nullptr;
    Lingmo::ui::Label* m_empty = nullptr;
    Lingmo::ui::Label* m_addCaption = nullptr;
    NumberStepper* m_hour = nullptr;
    NumberStepper* m_minute = nullptr;
    PillButton* m_add = nullptr;
    Lingmo::base::Timer m_tick;
    Lingmo::base::Settings m_settings;
    std::function<ClockReading()> m_nowSource;
    std::function<void(const std::string&)> m_onAlarm;
    bool m_active = false;
    int m_day = -1;
};

} // namespace Clock

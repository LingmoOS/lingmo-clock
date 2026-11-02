// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "base/timer.h"
#include "model/countdown_timer.h"
#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Label;
} // namespace Lingmo::ui

namespace Clock {

class NumberStepper;
class PillButton;

// The countdown timer: a ring that empties, the remaining time in its middle,
// the minutes and seconds set under it, and ready-made durations.
class TimerPage : public Lingmo::ui::View
{
public:
    TimerPage();
    ~TimerPage() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }

    void start();
    void pause();
    void toggle();
    void reset();
    void setMinutesSeconds(int minutes, int seconds);

    int minutes() const;
    int seconds() const;
    TimerState state() const { return m_timer.state(); }
    std::chrono::milliseconds duration() const { return m_timer.duration(); }
    std::chrono::milliseconds remaining() const { return m_timer.remaining(); }
    double progress() const { return m_timer.progress(); }
    std::string timeText() const;
    std::string captionText() const;

    // Called after the countdown ran out, for the window's notification.
    void setOnFinished(std::function<void()> callback) { m_onFinished = std::move(callback); }

    void retranslate();

    Lingmo::ui::SizeF sizeHint() const override;

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    class Dial;

    void updateDisplay();
    void updateButtons();
    void updateInputEnabled();
    void applyInputs();
    void finish();

    Dial* m_dial;
    NumberStepper* m_minutes;
    NumberStepper* m_seconds;
    Lingmo::ui::Label* m_minutesCaption;
    Lingmo::ui::Label* m_secondsCaption;
    PillButton* m_startPause;
    PillButton* m_reset;
    std::vector<PillButton*> m_presets;
    std::vector<int> m_presetMinutes;
    CountdownTimer m_timer;
    Lingmo::base::Timer m_tick;
    std::function<void()> m_onFinished;
    bool m_active = false;
    bool m_inputsRefreshing = false;
    bool m_finished = false;
};

} // namespace Clock

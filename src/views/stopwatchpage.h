// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "base/timer.h"
#include "model/stopwatch.h"
#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Column;
class Label;
class Row;
class ScrollView;
} // namespace Lingmo::ui

namespace Clock {

class PillButton;

// The stopwatch: start, pause, reset and laps, the time large in the middle.
class StopwatchPage : public Lingmo::ui::View
{
public:
    StopwatchPage();
    ~StopwatchPage() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }

    void start();
    void pause();
    void toggle();
    void reset();
    void addLap();

    bool isRunning() const { return m_stopwatch.isRunning(); }
    const Stopwatch& stopwatch() const { return m_stopwatch; }
    std::string timeText() const;
    size_t lapCount() const { return m_stopwatch.laps().size(); }
    // The split column's text of a lap, for tests.
    std::string lapSplitText(size_t index) const;

    void retranslate();

    Lingmo::ui::SizeF sizeHint() const override;

protected:
    void layout() override;
    void childrenLaidOut() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    struct LapRow;

    void updateDisplay();
    void updateButtons();
    void rebuildLaps();
    // The card the laps live in.
    Lingmo::ui::RectF lapsCard() const;

    Stopwatch m_stopwatch;
    Lingmo::ui::Label* m_time;
    PillButton* m_startPause;
    PillButton* m_lap;
    PillButton* m_reset;
    Lingmo::ui::Label* m_headingLap;
    Lingmo::ui::Label* m_headingSplit;
    Lingmo::ui::Label* m_headingTotal;
    Lingmo::ui::Label* m_empty;
    Lingmo::ui::ScrollView* m_scroll;
    Lingmo::ui::Column* m_lapList;
    std::vector<LapRow*> m_lapRows;
    Lingmo::base::Timer m_tick;
    bool m_active = false;
    bool m_pendingScrollToEnd = false;
};

} // namespace Clock

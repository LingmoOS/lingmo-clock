// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "model/clocktime.h"
#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Label;
class Painter;
} // namespace Lingmo::ui

namespace Clock {

// The round clock face: a dial with hour and minute ticks, the twelve
// numerals, and hands that carry each other on. The second hand sweeps with
// the fraction of the second it is given.
class AnalogClock : public Lingmo::ui::View
{
public:
    AnalogClock();
    ~AnalogClock() override;

    void setReading(const ClockReading& reading);
    const ClockReading& reading() const { return m_reading; }

    Lingmo::ui::SizeF sizeHint() const override;

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    void placeNumerals();

    ClockReading m_reading;
    std::vector<Lingmo::ui::Label*> m_numerals;
};

} // namespace Clock

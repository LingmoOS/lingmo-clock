// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/analogclock.h"

#include "model/timeformat.h"
#include "ui/canvas_2d.h"
#include "ui/color_id.h"
#include "ui/color_provider.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/path.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Lingmo::ui;

namespace Clock {

namespace {

constexpr int kNumeralCount = 12;
constexpr double kDialInset = 16;

ColorF toColorF(const Color& color)
{
    return {color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0};
}

PointF pointAt(const PointF& centre, double radius, double degrees)
{
    const double radians = degrees * std::numbers::pi / 180.0;
    return {centre.x + std::sin(radians) * radius, centre.y - std::cos(radians) * radius};
}

Color themed(ColorId id)
{
    return ColorProvider::current().get(id);
}

} // namespace

AnalogClock::AnalogClock()
{
    for (int i = 1; i <= kNumeralCount; ++i) {
        Label* numeral = addChild<Label>(std::to_string(i));
        numeral->setFontSize(15);
        numeral->setWeight(500);
        numeral->setColor(ColorRecipe(ColorId::Text));
        numeral->setTextAlignment(Label::TextAlignment::Center);
        numeral->setVerticallyCentered(true);
        m_numerals.push_back(numeral);
    }
}

AnalogClock::~AnalogClock() = default;

SizeF AnalogClock::sizeHint() const
{
    return {360, 360};
}

void AnalogClock::setReading(const ClockReading& reading)
{
    m_reading = reading;
    requestPaint();
}

void AnalogClock::layout()
{
    placeNumerals();
}

void AnalogClock::placeNumerals()
{
    const RectF rect = localRect();
    const double side = std::min(rect.width, rect.height);
    const double radius = std::max(0.0, side / 2 - kDialInset);
    const PointF centre{rect.width / 2, rect.height / 2};
    const double numeralRadius = radius * 0.78;
    const double boxWidth = std::max(26.0, side * 0.075);
    const double boxHeight = std::max(22.0, side * 0.066);
    for (int i = 1; i <= kNumeralCount; ++i) {
        const PointF position = pointAt(centre, numeralRadius, i * 30.0);
        m_numerals[size_t(i - 1)]->setGeometry(
            {position.x - boxWidth / 2, position.y - boxHeight / 2, boxWidth, boxHeight});
    }
}

void AnalogClock::paint(Painter& painter)
{
    const RectF rect = localRect();
    const double side = std::min(rect.width, rect.height);
    if (side < 60)
        return;
    const Theme& theme = Theme::current();
    const double radius = side / 2 - kDialInset;
    const PointF centre{rect.width / 2, rect.height / 2};
    const RectF dial{centre.x - radius, centre.y - radius, 2 * radius, 2 * radius};
    const double scale = painter.scale();

    // A soft drop shadow under the dial, then the dial itself in a gentle
    // radial gradient: the theme's card colour, lit from the middle.
    painter.drawShadow(dial, radius, 16, Color{0, 0, 0, uint8_t(theme.darkMode ? 90 : 26)});
    {
        Canvas2D canvas(painter);
        Path circle;
        circle.addEllipse(dial);
        canvas.clipToPath(circle);
        const Color inner = theme.secondBackground.lighter(1.03);
        const Color outer = theme.secondBackground.darker(theme.darkMode ? 1.22 : 1.02);
        canvas.fillRadialGradient({{0.0, toColorF(inner)},
                                   {0.72, toColorF(theme.secondBackground)},
                                   {1.0, toColorF(outer)}},
                                  centre, 0, centre, radius, false, true);
    }

    // A hairline where the dial's edge is, and the minutes' outer ring.
    Path edge;
    edge.addEllipse(dial);
    painter.strokePath(edge, theme.text.withAlpha(0.09), 1.0);
    const RectF ringRect{dial.x + 1.5, dial.y + 1.5, dial.width - 3, dial.height - 3};
    Path ring;
    ring.addEllipse(ringRect);
    painter.strokePath(ring, theme.text.withAlpha(0.045), 1.0);

    // The minute and hour ticks.
    const Color minuteInk = theme.text.withAlpha(0.26);
    const Color hourInk = theme.text.withAlpha(0.62);
    for (int i = 0; i < 60; ++i) {
        const bool hour = i % 5 == 0;
        const double from = hour ? radius - 17 : radius - 11;
        const double to = radius - 6;
        painter.drawLine(pointAt(centre, from, i * 6.0), pointAt(centre, to, i * 6.0),
                         hour ? hourInk : minuteInk, hour ? 1.7 * scale : 1.0 * scale);
    }

    const ClockHands hands = clockHands(m_reading.hour, m_reading.minute, m_reading.second,
                                        double(m_reading.nanosecond.count()) / 1e9);

    // The seconds' sweep along the rim, before the hands cover the middle.
    if (hands.second > 0.4) {
        const double rimInset = 2.5;
        const RectF rim{dial.x + rimInset, dial.y + rimInset, dial.width - 2 * rimInset,
                        dial.height - 2 * rimInset};
        Path sweep;
        sweep.arcTo(rim, -90.0, -hands.second);
        painter.strokePath(sweep, theme.highlight.withAlpha(0.55), 2.0, LineCap::Round);
    }

    const PointF shadowOffset{0.8, 1.6};
    const double hourLength = radius * 0.46;
    const double minuteLength = radius * 0.68;
    const double secondLength = radius * 0.80;
    const double secondTail = radius * 0.16;

    const PointF hourTip = pointAt(centre, hourLength, hands.hour);
    const PointF minuteTip = pointAt(centre, minuteLength, hands.minute);
    // A soft shadow under the two hands only.
    painter.drawLine(pointAt(centre, 0.0, hands.hour) + shadowOffset, hourTip + shadowOffset,
                     Color{0, 0, 0, uint8_t(theme.darkMode ? 70 : 22)}, 5.2 * scale);
    painter.drawLine(pointAt(centre, 0.0, hands.minute) + shadowOffset, minuteTip + shadowOffset,
                     Color{0, 0, 0, uint8_t(theme.darkMode ? 70 : 22)}, 3.4 * scale);

    painter.drawLine(centre, hourTip, theme.text, 5.4 * scale);
    painter.drawLine(centre, minuteTip, theme.text, 3.4 * scale);

    // The second hand, with its tail through the middle.
    const PointF secondTailPoint = pointAt(centre, -secondTail, hands.second);
    const PointF secondTip = pointAt(centre, secondLength, hands.second);
    painter.drawLine(secondTailPoint, secondTip, theme.highlight, 1.5 * scale);

    // The cap over the hands.
    painter.fillEllipse({centre.x - 6.5, centre.y - 6.5, 13, 13}, theme.highlight);
    painter.fillEllipse({centre.x - 2.8, centre.y - 2.8, 5.6, 5.6}, theme.secondBackground);
}

} // namespace Clock

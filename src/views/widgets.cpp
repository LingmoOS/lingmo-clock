// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/widgets.h"

#include "ui/color_id.h"
#include "ui/color_provider.h"
#include "ui/keys.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/text_field.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace Lingmo::ui;

namespace Clock {

static Color themed(ColorId id)
{
    return ColorProvider::current().get(id);
}

static bool activatesOnKey(const KeyEvent& event)
{
    return event.key == key::Space || event.key == key::Return || event.key == key::KeypadEnter;
}

// The soft focus ring controls draw with the keyboard's focus.
static void paintFocusRing(Painter& painter, const RectF& rect, double radius)
{
    if (!painter.opacity())
        return;
    painter.strokeRoundedRect(rect, radius, themed(ColorId::ButtonFocusRing), painter.borderWidth(2));
}

PillButton::PillButton(std::string text, Style style)
    : m_label(addChild<Label>(std::move(text)))
    , m_style(style)
{
    m_label->setTextAlignment(Label::TextAlignment::Center);
    m_label->setVerticallyCentered(true);
    setCursorShape(CursorShape::Pointer);
}

PillButton::~PillButton() = default;

void PillButton::setText(std::string text)
{
    m_label->setText(std::move(text));
    requestLayout();
}

void PillButton::setStyle(Style style)
{
    if (m_style == style)
        return;
    m_style = style;
    updateLabel();
    requestPaint();
}

void PillButton::setFontSize(double size)
{
    m_fontSize = size;
    updateLabel();
    requestLayout();
}

void PillButton::setMinimumWidth(double width)
{
    m_minimumWidth = width;
    requestLayout();
}

void PillButton::setCompact(bool compact)
{
    m_compact = compact;
    updateLabel();
    requestLayout();
}

void PillButton::updateLabel()
{
    m_label->setFontSize(m_fontSize > 0 ? m_fontSize : (m_compact ? 12.5 : 14.0));
    m_label->setWeight(600);
    // The accent's text is the highlighted text colour; the others follow the
    // theme's text colour.
    const std::optional<ColorRecipe> color =
        m_style == Style::Accent ? ColorRecipe(ColorId::HighlightedText) : ColorRecipe(ColorId::Text);
    m_label->setColor(color);
}

SizeF PillButton::sizeHint() const
{
    const SizeF label = m_label->sizeHint();
    const double pad = m_compact ? 13 : 22;
    const double height = m_compact ? 30 : 40;
    return {std::max(m_minimumWidth, label.width + 2 * pad), height};
}

void PillButton::layout()
{
    updateLabel();
    m_label->setGeometry(localRect());
}

void PillButton::paint(Painter& painter)
{
    updateLabel();
    const Theme& theme = Theme::current();
    const Corners corners = theme.continuousCorners ? Corners::Continuous : Corners::Circular;
    const RectF rect = localRect();
    const double radius = rect.height / 2;
    const bool enabled = isEnabled();

    Color fill;
    switch (m_style) {
    case Style::Accent:
        fill = themed(ColorId::Highlight);
        if (m_pressed)
            fill = fill.darker(1.18);
        else if (m_hovered)
            fill = fill.lighter(1.1);
        break;
    case Style::Secondary:
        fill = m_pressed ? themed(ColorId::ButtonBackgroundPressed)
            : m_hovered      ? themed(ColorId::ButtonBackgroundHovered)
                             : themed(ColorId::ButtonBackground);
        break;
    case Style::Ghost:
        fill = m_hovered ? themed(ColorId::AlternateBackground).withAlpha(m_pressed ? 0.9 : 0.6)
                         : Color{0, 0, 0, 0};
        break;
    }
    if (!enabled) {
        fill.a = uint8_t(fill.a * 0.6);
        painter.multiplyOpacity(0.55);
    }
    painter.fillRoundedRect(rect, radius, fill, corners);
    if (m_style != Style::Accent && (m_hovered || hasVisibleFocus()))
        painter.strokeRoundedRect(rect, radius, themed(ColorId::Separator), painter.borderWidth(1), corners);
    if (hasVisibleFocus())
        paintFocusRing(painter, rect.adjusted(-3, -3, 3, 3), radius + 3);
}

void PillButton::pointerEnter()
{
    m_hovered = true;
    requestPaint();
}

void PillButton::pointerLeave()
{
    m_hovered = false;
    m_pressed = false;
    requestPaint();
}

void PillButton::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left)
        return;
    m_pressed = true;
    requestPaint();
}

void PillButton::pointerRelease(const PointerEvent& event)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    requestPaint();
    if (localRect().contains(event.position) && onClicked && isEnabled())
        onClicked();
}

bool PillButton::keyPress(const KeyEvent& event)
{
    if (activatesOnKey(event)) {
        if (onClicked && isEnabled())
            onClicked();
        return true;
    }
    return false;
}

RoundButton::RoundButton(Glyph glyph, double diameter)
    : m_glyph(glyph)
    , m_diameter(diameter)
{
    setCursorShape(CursorShape::Pointer);
}

RoundButton::~RoundButton() = default;

void RoundButton::setGlyph(Glyph glyph)
{
    m_glyph = glyph;
    requestPaint();
}

void RoundButton::setDiameter(double diameter)
{
    m_diameter = diameter;
    requestLayout();
}

void RoundButton::setToolTipText(std::string text)
{
    setToolTip(std::move(text));
}

SizeF RoundButton::sizeHint() const
{
    return {m_diameter, m_diameter};
}

void RoundButton::paint(Painter& painter)
{
    const bool enabled = isEnabled();
    if (!enabled)
        painter.multiplyOpacity(0.5);
    const Theme& theme = Theme::current();
    const RectF rect = localRect();
    const double radius = std::min(rect.width, rect.height) / 2;
    if (m_hovered || m_pressed) {
        const Color fill = m_pressed ? themed(ColorId::RoundButtonPressed) : themed(ColorId::RoundButtonHovered);
        painter.fillRoundedRect(rect, radius, fill,
                                theme.continuousCorners ? Corners::Continuous : Corners::Circular);
    }
    if (hasVisibleFocus())
        paintFocusRing(painter, rect.adjusted(-2, -2, 2, 2), radius + 2);

    const PointF centre{rect.x + rect.width / 2, rect.y + rect.height / 2};
    const double arm = std::max(4.0, radius * 0.32);
    const Color ink = themed(ColorId::Text);
    const double width = std::max(1.5, 1.6 * painter.scale());
    switch (m_glyph) {
    case Glyph::Close:
        painter.drawLine({centre.x - arm, centre.y - arm}, {centre.x + arm, centre.y + arm}, ink, width);
        painter.drawLine({centre.x + arm, centre.y - arm}, {centre.x - arm, centre.y + arm}, ink, width);
        break;
    case Glyph::Plus:
        painter.drawLine({centre.x - arm, centre.y}, {centre.x + arm, centre.y}, ink, width);
        painter.drawLine({centre.x, centre.y - arm}, {centre.x, centre.y + arm}, ink, width);
        break;
    case Glyph::Minus:
        painter.drawLine({centre.x - arm, centre.y}, {centre.x + arm, centre.y}, ink, width);
        break;
    }
}

void RoundButton::pointerEnter()
{
    m_hovered = true;
    requestPaint();
}

void RoundButton::pointerLeave()
{
    m_hovered = false;
    m_pressed = false;
    requestPaint();
}

void RoundButton::pointerPress(const PointerEvent& event)
{
    if (event.button != MouseButton::Left)
        return;
    m_pressed = true;
    requestPaint();
}

void RoundButton::pointerRelease(const PointerEvent& event)
{
    if (!m_pressed)
        return;
    m_pressed = false;
    requestPaint();
    if (localRect().contains(event.position) && onClicked && isEnabled())
        onClicked();
}

bool RoundButton::keyPress(const KeyEvent& event)
{
    if (activatesOnKey(event)) {
        if (onClicked && isEnabled())
            onClicked();
        return true;
    }
    return false;
}

NumberStepper::NumberStepper(int minimum, int maximum, int value)
    : m_minus(addChild<RoundButton>(RoundButton::Glyph::Minus, 30))
    , m_plus(addChild<RoundButton>(RoundButton::Glyph::Plus, 30))
    , m_field(addChild<TextField>())
    , m_minimum(minimum)
    , m_maximum(std::max(minimum, maximum))
    , m_value(std::clamp(value, std::min(minimum, maximum), std::max(minimum, maximum)))
{
    m_minus->onClicked = [this] { step(-1); };
    m_plus->onClicked = [this] { step(1); };
    m_field->setFrameVisible(false);
    m_field->setHorizontallyCentered(true);
    m_field->setFontSize(18);
    m_field->setFontWeight(600);
    m_field->setTextMargins(0, 0);
    m_field->setInputPurpose(TextPurpose::Number);
    m_field->setMaximumLength(3);
    m_field->setValidator([](const std::string& text) {
        if (text.size() > 3)
            return false;
        return std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; });
    });
    m_field->onTextEdited = [this] { edited(); };
    m_field->onEditingFinished = [this] { normalize(); };
    refreshText();
}

NumberStepper::~NumberStepper() = default;

void NumberStepper::setValue(int value)
{
    const int clamped = std::clamp(value, m_minimum, m_maximum);
    if (clamped == m_value)
        return;
    m_value = clamped;
    refreshText();
    if (onValueChanged)
        onValueChanged(m_value);
}

void NumberStepper::step(int delta)
{
    setValue(m_value + delta);
}

void NumberStepper::edited()
{
    if (m_refreshing)
        return;
    const std::string& text = m_field->text();
    if (text.empty())
        return;
    const int typed = std::atoi(text.c_str());
    if (typed < m_minimum || typed > m_maximum)
        return;
    if (typed == m_value)
        return;
    m_value = typed;
    if (onValueChanged)
        onValueChanged(m_value);
}

void NumberStepper::normalize()
{
    m_refreshing = true;
    m_field->setText(std::to_string(m_value));
    m_refreshing = false;
}

void NumberStepper::refreshText()
{
    if (!m_field)
        return;
    m_refreshing = true;
    m_field->setText(std::to_string(m_value));
    m_refreshing = false;
}

SizeF NumberStepper::sizeHint() const
{
    return {168, 40};
}

void NumberStepper::layout()
{
    const RectF rect = localRect();
    const double button = 30;
    const double middle = std::max(0.0, rect.width - 2 * button - 2 * 8);
    m_minus->setGeometry({8, (rect.height - button) / 2, button, button});
    m_field->setGeometry({8 + button + 4, 0, middle + 8, rect.height});
    m_plus->setGeometry({rect.width - 8 - button, (rect.height - button) / 2, button, button});
}

void NumberStepper::paint(Painter& painter)
{
    const Theme& theme = Theme::current();
    const Corners corners = theme.continuousCorners ? Corners::Continuous : Corners::Circular;
    const RectF rect = localRect();
    const double radius = rect.height / 2;
    painter.fillRoundedRect(rect, radius, themed(ColorId::ButtonBackground), corners);
    painter.strokeRoundedRect(rect, radius, themed(ColorId::Separator), painter.borderWidth(1), corners);
}

} // namespace Clock

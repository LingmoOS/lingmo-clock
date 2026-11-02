// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>

#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Label;
class TextField;
} // namespace Lingmo::ui

namespace Clock {

// A pill-shaped text button: an accent-filled action, a tonal secondary one or
// a borderless ghost.
class PillButton : public Lingmo::ui::View
{
public:
    enum class Style {
        Accent,
        Secondary,
        Ghost,
    };

    explicit PillButton(std::string text, Style style = Style::Accent);
    ~PillButton() override;

    void setText(std::string text);
    void setStyle(Style style);
    // Logical pixels; 0 follows the theme.
    void setFontSize(double size);
    void setMinimumWidth(double width);
    // Smaller pads and a shorter pill, for preset rows.
    void setCompact(bool compact);

    std::function<void()> onClicked;

    Lingmo::ui::SizeF sizeHint() const override;
    bool acceptsPointer() const override { return true; }
    bool acceptsFocus() const override { return true; }

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;
    void pointerEnter() override;
    void pointerLeave() override;
    void pointerPress(const Lingmo::ui::PointerEvent& event) override;
    void pointerRelease(const Lingmo::ui::PointerEvent& event) override;
    bool keyPress(const Lingmo::ui::KeyEvent& event) override;

private:
    void updateLabel();

    Lingmo::ui::Label* m_label;
    Style m_style;
    double m_fontSize = 0;
    double m_minimumWidth = 0;
    bool m_compact = false;
    bool m_hovered = false;
    bool m_pressed = false;
};

// A small round button with a drawn glyph: close, plus or minus.
class RoundButton : public Lingmo::ui::View
{
public:
    enum class Glyph {
        Close,
        Plus,
        Minus,
    };

    explicit RoundButton(Glyph glyph, double diameter = 30);
    ~RoundButton() override;

    void setGlyph(Glyph glyph);
    void setDiameter(double diameter);
    void setToolTipText(std::string text);

    std::function<void()> onClicked;

    Lingmo::ui::SizeF sizeHint() const override;
    bool acceptsPointer() const override { return true; }
    bool acceptsFocus() const override { return true; }

protected:
    void paint(Lingmo::ui::Painter& painter) override;
    void pointerEnter() override;
    void pointerLeave() override;
    void pointerPress(const Lingmo::ui::PointerEvent& event) override;
    void pointerRelease(const Lingmo::ui::PointerEvent& event) override;
    bool keyPress(const Lingmo::ui::KeyEvent& event) override;

private:
    Glyph m_glyph;
    double m_diameter;
    bool m_hovered = false;
    bool m_pressed = false;
};

// A number a timer is set with: "− 25 +", the number typed into as well as
// stepped.
class NumberStepper : public Lingmo::ui::View
{
public:
    NumberStepper(int minimum, int maximum, int value = 0);
    ~NumberStepper() override;

    int value() const { return m_value; }
    void setValue(int value);
    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }

    // After the number changed, by the user or by setValue().
    std::function<void(int)> onValueChanged;

    Lingmo::ui::SizeF sizeHint() const override;
    bool acceptsPointer() const override { return false; }

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    void step(int delta);
    void edited();
    void normalize();
    void refreshText();

    RoundButton* m_minus;
    RoundButton* m_plus;
    Lingmo::ui::TextField* m_field;
    int m_minimum;
    int m_maximum;
    int m_value;
    bool m_refreshing = false;
};

} // namespace Clock

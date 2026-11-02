// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/timerpage.h"

#include "views/widgets.h"

#include "base/i18n.h"
#include "model/timeformat.h"
#include "ui/color_id.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/path.h"
#include "ui/text_layout.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>

using namespace Lingmo::ui;
using namespace std::chrono_literals;
using Lingmo::i18n::tr;

namespace Clock {

namespace {

constexpr double kRingWidth = 12;
constexpr double kDialInset = 12;
constexpr double kMargin = 30;

} // namespace

// The ring with the time in the middle.
class TimerPage::Dial : public Lingmo::ui::View
{
public:
    Dial()
        : time(addChild<Label>())
        , caption(addChild<Label>())
    {
        time->setName("timerTime");
        time->setFontFamily(systemFixedFont().family);
        time->setFontSize(52);
        time->setWeight(300);
        time->setTextAlignment(Label::TextAlignment::Center);
        time->setVerticallyCentered(true);

        caption->setName("timerCaption");
        caption->setFontSize(13);
        caption->setWeight(500);
        caption->setColor(ColorRecipe(ColorId::DisabledText));
        caption->setTextAlignment(Label::TextAlignment::Center);
        caption->setVerticallyCentered(true);
    }

    void setProgress(double progress)
    {
        if (m_progress == progress)
            return;
        m_progress = std::clamp(progress, 0.0, 1.0);
        requestPaint();
    }
    double progress() const { return m_progress; }

    void setFinished(bool finished)
    {
        if (m_finished == finished)
            return;
        m_finished = finished;
        requestPaint();
    }

    Label* time;
    Label* caption;

    SizeF sizeHint() const override { return {300, 300}; }

protected:
    void layout() override
    {
        const RectF rect = localRect();
        const double timeHeight = std::max(40.0, time->sizeHint().height);
        const double captionHeight = std::max(16.0, caption->sizeHint().height);
        const double centre = rect.height / 2;
        time->setGeometry({0, centre - timeHeight - 2, rect.width, timeHeight});
        caption->setGeometry({0, centre + 6, rect.width, captionHeight});
    }

    void paint(Painter& painter) override
    {
        const RectF rect = localRect();
        const double side = std::min(rect.width, rect.height);
        if (side < 80)
            return;
        const Theme& theme = Theme::current();
        const double radius = side / 2 - kDialInset;
        const PointF centre{rect.width / 2, rect.height / 2};
        const RectF ring{centre.x - radius, centre.y - radius, 2 * radius, 2 * radius};

        const Color track = theme.text.withAlpha(0.08);
        Path trackPath;
        trackPath.addEllipse(ring);
        painter.strokePath(trackPath, track, kRingWidth);

        const Color ink = m_finished ? accent::red : theme.highlight;
        if (m_progress >= 0.9999) {
            Path full;
            full.addEllipse(ring);
            painter.strokePath(full, ink, kRingWidth, LineCap::Round);
        } else if (m_progress > 0.0015) {
            // From the top, clockwise: a negative sweep in path angles.
            Path arc;
            arc.arcTo(ring, -90.0, -360.0 * m_progress);
            painter.strokePath(arc, ink, kRingWidth, LineCap::Round);
        }

        // A knob at the end of the arc, so the sweep reads at a glance.
        const double angle = -90.0 + 360.0 * m_progress;
        const double radians = angle * std::numbers::pi / 180.0;
        const PointF knob{centre.x + std::cos(radians) * radius,
                          centre.y + std::sin(radians) * radius};
        painter.fillEllipse({knob.x - 5, knob.y - 5, 10, 10}, ink);
        painter.fillEllipse({knob.x - 2, knob.y - 2, 4, 4}, theme.secondBackground);
    }

private:
    double m_progress = 0;
    bool m_finished = false;
};

TimerPage::TimerPage()
    : m_dial(addChild<Dial>())
    , m_minutes(addChild<NumberStepper>(0, 599, 5))
    , m_seconds(addChild<NumberStepper>(0, 59, 0))
    , m_minutesCaption(addChild<Label>())
    , m_secondsCaption(addChild<Label>())
    , m_startPause(addChild<PillButton>(tr("Start"), PillButton::Style::Accent))
    , m_reset(addChild<PillButton>(tr("Reset"), PillButton::Style::Secondary))
{
    setName("timerPage");
    m_dial->setName("timerDial");

    for (Label* caption : {m_minutesCaption, m_secondsCaption}) {
        caption->setFontSize(13);
        caption->setWeight(500);
        caption->setColor(ColorRecipe(ColorId::DisabledText));
        caption->setVerticallyCentered(true);
    }

    m_startPause->setName("timerStartPauseButton");
    m_reset->setName("timerResetButton");
    m_startPause->onClicked = [this] { toggle(); };
    m_reset->onClicked = [this] { reset(); };

    m_minutes->onValueChanged = [this](int) { applyInputs(); };
    m_seconds->onValueChanged = [this](int) { applyInputs(); };

    for (int minutes : {1, 3, 5, 10, 15, 25}) {
        PillButton* preset = addChild<PillButton>(std::string(), PillButton::Style::Secondary);
        preset->setCompact(true);
        preset->onClicked = [this, minutes] { setMinutesSeconds(minutes, 0); };
        m_presets.push_back(preset);
        m_presetMinutes.push_back(minutes);
    }

    m_timer.setDuration(5min);
    m_timer.onFinished = [this] { finish(); };
    m_tick.setInterval(100ms);
    m_tick.onTimeout = [this] {
        if (m_timer.update()) {
            m_tick.stop();
            return;
        }
        updateDisplay();
    };

    retranslate();
    updateDisplay();
    updateButtons();
}

TimerPage::~TimerPage() = default;

SizeF TimerPage::sizeHint() const
{
    return {760, 520};
}

void TimerPage::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (active && m_timer.isRunning())
        m_tick.start();
    else
        m_tick.stop();
}

int TimerPage::minutes() const
{
    return m_minutes->value();
}

int TimerPage::seconds() const
{
    return m_seconds->value();
}

std::string TimerPage::timeText() const
{
    return m_dial->time->text();
}

std::string TimerPage::captionText() const
{
    return m_dial->caption->text();
}

void TimerPage::setMinutesSeconds(int minutes, int seconds)
{
    m_inputsRefreshing = true;
    m_minutes->setValue(minutes);
    m_seconds->setValue(seconds);
    m_inputsRefreshing = false;
    if (m_timer.isRunning())
        pause();
    m_timer.setDuration(std::chrono::milliseconds((minutes * 60 + seconds) * 1000));
    m_finished = false;
    m_dial->setFinished(false);
    m_dial->setProgress(0);
    m_tick.stop();
    updateDisplay();
    updateButtons();
    updateInputEnabled();
}

void TimerPage::start()
{
    if (m_timer.duration() <= 0ms)
        return;
    if (m_timer.state() == TimerState::Finished) {
        m_finished = false;
        m_dial->setFinished(false);
    }
    m_timer.start();
    m_tick.start();
    updateButtons();
    updateInputEnabled();
    updateDisplay();
}

void TimerPage::pause()
{
    m_timer.pause();
    m_tick.stop();
    updateButtons();
    updateInputEnabled();
    updateDisplay();
}

void TimerPage::toggle()
{
    if (m_timer.isRunning())
        pause();
    else
        start();
}

void TimerPage::reset()
{
    m_tick.stop();
    m_timer.reset();
    m_finished = false;
    m_dial->setFinished(false);
    m_dial->setProgress(0);
    updateButtons();
    updateInputEnabled();
    updateDisplay();
}

void TimerPage::applyInputs()
{
    if (m_inputsRefreshing)
        return;
    if (m_timer.isRunning())
        return;
    const int total = m_minutes->value() * 60 + m_seconds->value();
    if (total == 0)
        return;
    m_timer.setDuration(std::chrono::milliseconds(total * 1000));
    m_finished = false;
    m_dial->setFinished(false);
    m_dial->setProgress(0);
    updateDisplay();
    updateButtons();
}

void TimerPage::finish()
{
    m_finished = true;
    m_dial->setFinished(true);
    m_tick.stop();
    updateDisplay();
    updateButtons();
    updateInputEnabled();
    if (m_onFinished)
        m_onFinished();
}

void TimerPage::updateDisplay()
{
    const std::chrono::milliseconds remaining = m_timer.remaining();
    m_dial->time->setText(formatCountdown(remaining));
    m_dial->setProgress(m_timer.progress());
    switch (m_timer.state()) {
    case TimerState::Idle:
        m_dial->caption->setText(tr("Ready"));
        break;
    case TimerState::Running:
        m_dial->caption->setText(tr("Remaining"));
        break;
    case TimerState::Paused:
        m_dial->caption->setText(tr("Paused"));
        break;
    case TimerState::Finished:
        m_dial->caption->setText(tr("Time's up!"));
        break;
    }
}

void TimerPage::updateButtons()
{
    m_startPause->setText(m_timer.isRunning() ? tr("Pause") : tr("Start"));
    m_startPause->setEnabled(m_timer.duration() > 0ms);
    m_reset->setEnabled(m_timer.state() != TimerState::Idle || m_timer.remaining() < m_timer.duration());
}

void TimerPage::updateInputEnabled()
{
    const bool editable = !m_timer.isRunning();
    m_minutes->setEnabled(editable);
    m_seconds->setEnabled(editable);
    for (PillButton* preset : m_presets)
        preset->setEnabled(editable);
}

void TimerPage::retranslate()
{
    m_minutesCaption->setText(tr("Minutes"));
    m_secondsCaption->setText(tr("Seconds"));
    m_reset->setText(tr("Reset"));
    for (size_t i = 0; i < m_presets.size(); ++i) {
        m_presets[i]->setText(Lingmo::i18n::format(tr("{n} min"),
                                                   {{"n", std::to_string(m_presetMinutes[i])}}));
        m_presets[i]->setToolTip({});
    }
    m_startPause->setText(m_timer.isRunning() ? tr("Pause") : tr("Start"));
    updateDisplay();
    requestLayout();
}

void TimerPage::layout()
{
    const RectF rect = localRect();
    const double width = rect.width;
    const double height = rect.height;
    const double dialSide = std::max(160.0, std::min(360.0, std::min(width - 240.0, height * 0.50)));
    const double presetHeight = 30;
    const double stepperHeight = 40;
    const double buttonHeight = 40;
    const double gaps = 14 + 16 + 16;
    const double total = dialSide + presetHeight + stepperHeight + buttonHeight + gaps;
    double y = std::max(6.0, (height - total) / 2);

    m_dial->setGeometry({(width - dialSide) / 2, y, dialSide, dialSide});
    y += dialSide + 14;

    // The preset durations, centred as a row.
    double presetWidth = 0;
    for (PillButton* preset : m_presets)
        presetWidth += preset->sizeHint().width;
    presetWidth += double(m_presets.size() - 1) * 8;
    double x = std::max(0.0, (width - presetWidth) / 2);
    for (PillButton* preset : m_presets) {
        const SizeF hint = preset->sizeHint();
        preset->setGeometry({x, y, hint.width, presetHeight});
        x += hint.width + 8;
    }
    y += presetHeight + 16;

    // Minutes and seconds, each with its caption beside it.
    const double stepperWidth = 168;
    const double captionWidth = 66;
    const double groupWidth = captionWidth + 8 + stepperWidth;
    const double steppersWidth = 2 * groupWidth + 34;
    x = std::max(0.0, (width - steppersWidth) / 2);
    m_minutesCaption->setTextAlignment(Label::TextAlignment::Right);
    m_minutesCaption->setGeometry({x, y + (stepperHeight - 18) / 2, captionWidth, 18});
    m_minutes->setGeometry({x + captionWidth + 8, y, stepperWidth, stepperHeight});
    x += groupWidth + 34;
    m_secondsCaption->setTextAlignment(Label::TextAlignment::Right);
    m_secondsCaption->setGeometry({x, y + (stepperHeight - 18) / 2, captionWidth, 18});
    m_seconds->setGeometry({x + captionWidth + 8, y, stepperWidth, stepperHeight});
    y += stepperHeight + 16;

    const double buttonsWidth =
        m_startPause->sizeHint().width + m_reset->sizeHint().width + 12;
    x = std::max(0.0, (width - buttonsWidth) / 2);
    m_startPause->setGeometry({x, y, m_startPause->sizeHint().width, buttonHeight});
    x += m_startPause->sizeHint().width + 12;
    m_reset->setGeometry({x, y, m_reset->sizeHint().width, buttonHeight});
}

void TimerPage::paint(Painter& painter)
{
    (void)painter;
}

} // namespace Clock

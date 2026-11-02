// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/stopwatchpage.h"

#include "views/widgets.h"

#include "base/i18n.h"
#include "model/timeformat.h"
#include "ui/color_id.h"
#include "ui/label.h"
#include "ui/layout.h"
#include "ui/painter.h"
#include "ui/scroll_view.h"
#include "ui/text_layout.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>
#include <optional>

using namespace Lingmo::ui;
using namespace std::chrono_literals;
using Lingmo::i18n::tr;

namespace Clock {

namespace {

constexpr double kMargin = 28;
constexpr double kButtonSpacing = 10;
constexpr double kLapRowHeight = 30;
constexpr double kCardHeadingHeight = 18;

Label* addText(View* parent, std::string text, double fontSize, int weight,
               std::optional<ColorRecipe> color)
{
    Label* label = parent->addChild<Label>(std::move(text));
    label->setFontSize(fontSize);
    label->setWeight(weight);
    if (color)
        label->setColor(color);
    return label;
}

} // namespace

struct StopwatchPage::LapRow
{
    Row* row = nullptr;
    Label* index = nullptr;
    Label* split = nullptr;
    Label* total = nullptr;
};

StopwatchPage::StopwatchPage()
    : m_time(addChild<Label>())
    , m_startPause(addChild<PillButton>(tr("Start"), PillButton::Style::Accent))
    , m_lap(addChild<PillButton>(tr("Lap"), PillButton::Style::Secondary))
    , m_reset(addChild<PillButton>(tr("Reset"), PillButton::Style::Secondary))
    , m_headingLap(addChild<Label>())
    , m_headingSplit(addChild<Label>())
    , m_headingTotal(addChild<Label>())
    , m_scroll(addChild<ScrollView>())
    , m_lapList(nullptr)
{
    setName("stopwatchPage");

    m_time->setName("stopwatchTime");
    m_time->setFontFamily(systemFixedFont().family);
    m_time->setFontSize(82);
    m_time->setWeight(300);
    m_time->setTextAlignment(Label::TextAlignment::Center);
    m_time->setVerticallyCentered(true);
    m_time->setText("00:00.00");

    m_startPause->setName("startPauseButton");
    m_lap->setName("lapButton");
    m_reset->setName("resetButton");
    m_startPause->onClicked = [this] { toggle(); };
    m_lap->onClicked = [this] { addLap(); };
    m_reset->onClicked = [this] { reset(); };

    for (Label* heading : {m_headingLap, m_headingSplit, m_headingTotal}) {
        heading->setName("lapHeading");
        heading->setFontSize(12.5);
        heading->setWeight(500);
        heading->setColor(ColorRecipe(ColorId::DisabledText));
        heading->setVerticallyCentered(true);
    }

    m_scroll->setName("lapsScroll");
    m_scroll->setVerticalBarPadding(4, 4);
    m_scroll->setElastic(false);
    auto column = std::make_unique<Column>();
    column->setSpacing(2);
    column->setName("lapList");
    m_lapList = column.get();
    m_scroll->setContentView(std::move(column));

    m_empty = m_lapList->addChild<Label>(tr("No laps yet"));
    m_empty->setFontSize(14);
    m_empty->setColor(ColorRecipe(ColorId::DisabledText));
    m_empty->setTextAlignment(Label::TextAlignment::Center);
    m_empty->setVerticallyCentered(true);
    m_empty->setLayoutHints({.fillWidth = true, .preferredHeight = 60});

    m_tick.setInterval(40ms);
    m_tick.onTimeout = [this] { updateDisplay(); };
    retranslate();
    updateDisplay();
    updateButtons();
}

StopwatchPage::~StopwatchPage()
{
    for (LapRow* row : m_lapRows)
        delete row;
}

SizeF StopwatchPage::sizeHint() const
{
    return {760, 520};
}

void StopwatchPage::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (active && m_stopwatch.isRunning())
        m_tick.start();
    else
        m_tick.stop();
}

void StopwatchPage::start()
{
    m_stopwatch.start();
    m_tick.start();
    updateButtons();
    updateDisplay();
}

void StopwatchPage::pause()
{
    m_stopwatch.pause();
    m_tick.stop();
    updateButtons();
    updateDisplay();
}

void StopwatchPage::toggle()
{
    if (m_stopwatch.isRunning())
        pause();
    else
        start();
}

void StopwatchPage::reset()
{
    m_stopwatch.reset();
    m_tick.stop();
    m_pendingScrollToEnd = false;
    rebuildLaps();
    updateButtons();
    updateDisplay();
}

void StopwatchPage::addLap()
{
    if (!m_stopwatch.addLap())
        return;
    rebuildLaps();
    m_pendingScrollToEnd = true;
    requestLayout();
    updateButtons();
}

std::string StopwatchPage::timeText() const
{
    return m_time->text();
}

std::string StopwatchPage::lapSplitText(size_t index) const
{
    return index < m_lapRows.size() ? m_lapRows[index]->split->text() : std::string();
}

void StopwatchPage::updateDisplay()
{
    m_time->setText(formatStopwatch(m_stopwatch.elapsed()));
}

void StopwatchPage::updateButtons()
{
    m_startPause->setText(m_stopwatch.isRunning() ? tr("Pause") : tr("Start"));
    const bool started = m_stopwatch.isStarted();
    m_lap->setEnabled(started);
    m_reset->setEnabled(started || !m_stopwatch.laps().empty());
}

void StopwatchPage::retranslate()
{
    m_startPause->setText(m_stopwatch.isRunning() ? tr("Pause") : tr("Start"));
    m_lap->setText(tr("Lap"));
    m_reset->setText(tr("Reset"));
    m_headingLap->setText(tr("Lap"));
    m_headingSplit->setText(tr("Lap time"));
    m_headingTotal->setText(tr("Total time"));
    m_empty->setText(tr("No laps yet"));
    requestLayout();
}

void StopwatchPage::rebuildLaps()
{
    for (LapRow* row : m_lapRows) {
        m_lapList->takeChild(row->row);
        delete row;
    }
    m_lapRows.clear();

    const std::vector<Lap>& laps = m_stopwatch.laps();
    m_empty->setVisible(laps.empty());
    for (const Lap& lap : laps) {
        Row* row = m_lapList->addChild<Row>();
        row->setName("lapRow");
        row->setLayoutHints({.fillWidth = true, .preferredHeight = kLapRowHeight});
        LapRow* lapRow = new LapRow;
        lapRow->row = row;
        lapRow->index = addText(row, "#" + std::to_string(lap.index), 13.5, 500,
                                ColorRecipe(ColorId::DisabledText));
        lapRow->index->setLayoutHints({.alignment = Alignment::Start, .preferredWidth = 62});
        lapRow->split = addText(row, formatStopwatch(lap.split), 14, 500, std::nullopt);
        lapRow->split->setFontFamily(systemFixedFont().family);
        lapRow->split->setLayoutHints({.fillWidth = true});
        lapRow->total = addText(row, formatStopwatch(lap.total), 13.5, 400,
                               ColorRecipe(ColorId::DisabledText));
        lapRow->total->setFontFamily(systemFixedFont().family);
        lapRow->total->setTextAlignment(Label::TextAlignment::Right);
        lapRow->total->setLayoutHints({.alignment = Alignment::End, .preferredWidth = 106});
        if (lap.best)
            lapRow->split->setColor(ColorRecipe(accent::green));
        else if (lap.worst)
            lapRow->split->setColor(ColorRecipe(accent::red));
        m_lapRows.push_back(lapRow);
    }
    m_lapList->requestLayout();
}

RectF StopwatchPage::lapsCard() const
{
    const RectF rect = localRect();
    const double top = m_time->geometry().bottom() + m_startPause->geometry().height + 34;
    return {kMargin, top, std::max(0.0, rect.width - 2 * kMargin),
            std::max(0.0, rect.height - top - 10)};
}

void StopwatchPage::layout()
{
    const RectF rect = localRect();
    const double margin = kMargin;
    const double timeHeight = std::max(80.0, m_time->sizeHint().height);
    const double top = 22;
    m_time->setGeometry({margin, top, std::max(0.0, rect.width - 2 * margin), timeHeight});

    const double buttonTop = top + timeHeight + 10;
    const double totalWidth = m_startPause->sizeHint().width + m_lap->sizeHint().width
        + m_reset->sizeHint().width + 2 * kButtonSpacing;
    double x = std::max(0.0, (rect.width - totalWidth) / 2);
    for (PillButton* button : {m_startPause, m_lap, m_reset}) {
        const SizeF hint = button->sizeHint();
        button->setGeometry({x, buttonTop, hint.width, hint.height});
        x += hint.width + kButtonSpacing;
    }

    const RectF card = lapsCard();
    m_scroll->setGeometry({card.x + 12, card.y + 46, std::max(0.0, card.width - 24),
                           std::max(0.0, card.height - 54)});
    const double headingTop = card.y + 12;
    m_headingLap->setGeometry({card.x + 24, headingTop, 80, kCardHeadingHeight});
    m_headingSplit->setGeometry(
        {card.x + 110, headingTop, std::max(0.0, card.width - 110 - 128), kCardHeadingHeight});
    m_headingTotal->setGeometry(
        {card.right() - 128, headingTop, 106, kCardHeadingHeight});
    m_headingLap->setTextAlignment(Label::TextAlignment::Left);
    m_headingSplit->setTextAlignment(Label::TextAlignment::Center);
    m_headingTotal->setTextAlignment(Label::TextAlignment::Right);
}

void StopwatchPage::childrenLaidOut()
{
    if (!m_pendingScrollToEnd || m_lapRows.empty())
        return;
    m_pendingScrollToEnd = false;
    m_scroll->ensureVisible(m_lapRows.back()->row->geometry());
}

void StopwatchPage::paint(Painter& painter)
{
    const Theme& theme = Theme::current();
    const Corners corners = theme.continuousCorners ? Corners::Continuous : Corners::Circular;
    const RectF card = lapsCard();
    if (card.width <= 0 || card.height <= 0)
        return;
    painter.drawShadow(card, theme.bigRadius, 10, Color{0, 0, 0, uint8_t(theme.darkMode ? 70 : 16)},
                       {0, 2});
    painter.fillRoundedRect(card, theme.bigRadius, theme.secondBackground, corners);
    painter.strokeRoundedRect(card, theme.bigRadius, theme.text.withAlpha(0.06), painter.borderWidth(1),
                              corners);
    painter.drawHorizontalLine(card.x + 20, card.y + 42, std::max(0.0, card.width - 40),
                               theme.text.withAlpha(0.08), painter.borderWidth(1));
}

} // namespace Clock

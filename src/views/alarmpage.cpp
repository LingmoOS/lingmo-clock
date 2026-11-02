// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/alarmpage.h"

#include "views/widgets.h"

#include "base/i18n.h"
#include "ui/color_id.h"
#include "ui/label.h"
#include "ui/layout.h"
#include "ui/painter.h"
#include "ui/scroll_view.h"
#include "ui/theme.h"

#include <algorithm>
#include <cstdio>

using namespace Lingmo::ui;
using Lingmo::i18n::tr;

namespace Clock {

class AlarmPage::AlarmRow : public View
{
public:
    AlarmRow(AlarmPage* page, size_t index, const Alarm& alarm)
        : m_page(page)
        , m_index(index)
        , m_time(addChild<Label>(formatAlarm(alarm.hour, alarm.minute)))
        , m_caption(addChild<Label>(alarm.enabled ? tr("Alarm on") : tr("Alarm off")))
        , m_toggle(addChild<PillButton>(alarm.enabled ? tr("On") : tr("Off"),
                                       PillButton::Style::Secondary))
        , m_remove(addChild<RoundButton>(RoundButton::Glyph::Close, 28))
    {
        setName("alarmRow");
        setLayoutHints({.fillWidth = true});
        m_time->setFontSize(25);
        m_time->setWeight(500);
        m_time->setVerticallyCentered(true);
        m_caption->setFontSize(12);
        m_caption->setColor(ColorRecipe(ColorId::DisabledText));
        m_caption->setVerticallyCentered(true);
        m_toggle->setCompact(true);
        m_toggle->setMinimumWidth(64);
        m_toggle->setName("alarmToggle");
        m_toggle->onClicked = [this] {
            m_page->setAlarmEnabled(m_index, !m_page->alarmEnabled(m_index));
            m_page->rebuildRows();
        };
        m_remove->setName("removeAlarmButton");
        m_remove->onClicked = [this] {
            m_page->removeAlarm(m_index);
            m_page->rebuildRows();
        };
    }

    SizeF sizeHint() const override { return {0, 64}; }

protected:
    void layout() override
    {
        const RectF rect = localRect();
        const double right = rect.width - 18 - 28 - 14 - 80;
        m_time->setGeometry({24, 13, 120, 34});
        m_caption->setGeometry({24, 40, std::max(0.0, right - 24), 16});
        m_toggle->setGeometry({rect.width - 18 - 28 - 14 - 70, (rect.height - 32) / 2, 70, 32});
        const SizeF removeHint = m_remove->sizeHint();
        m_remove->setGeometry({rect.width - 18 - removeHint.width, (rect.height - removeHint.height) / 2,
                               removeHint.width, removeHint.height});
    }

    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        const RectF rect = localRect();
        painter.fillRoundedRect(rect, theme.bigRadius, theme.secondBackground);
        painter.strokeRoundedRect(rect, theme.bigRadius, theme.text.withAlpha(0.05),
                                  painter.borderWidth(1));
    }

private:
    AlarmPage* m_page;
    size_t m_index;
    Label* m_time;
    Label* m_caption;
    PillButton* m_toggle;
    RoundButton* m_remove;
};

AlarmPage::AlarmPage()
    : m_scroll(addChild<ScrollView>())
    , m_empty(addChild<Label>())
    , m_addCaption(addChild<Label>())
    , m_hour(addChild<NumberStepper>(0, 23, 7))
    , m_minute(addChild<NumberStepper>(0, 59, 0))
    , m_add(addChild<PillButton>(tr("Add"), PillButton::Style::Accent))
{
    setName("alarmPage");
    m_scroll->setName("alarmScroll");
    m_scroll->setVerticalBarPadding(4, 4);
    m_scroll->setElastic(false);
    auto column = std::make_unique<Column>();
    column->setSpacing(8);
    m_list = column.get();
    m_scroll->setContentView(std::move(column));

    m_empty->setName("emptyAlarms");
    m_empty->setFontSize(14);
    m_empty->setColor(ColorRecipe(ColorId::DisabledText));
    m_empty->setTextAlignment(Label::TextAlignment::Center);
    m_empty->setVerticallyCentered(true);

    m_addCaption->setText(tr("New alarm"));
    m_addCaption->setFontSize(13);
    m_addCaption->setColor(ColorRecipe(ColorId::DisabledText));
    m_hour->setName("alarmHour");
    m_minute->setName("alarmMinute");
    m_add->setName("addAlarmButton");
    m_add->setCompact(true);
    m_add->setMinimumWidth(86);
    m_add->onClicked = [this] { addFromInputs(); };

    m_tick.setInterval(std::chrono::seconds(1));
    m_tick.onTimeout = [this] { checkAlarms(); };

    addAlarm(7, 0);
    rebuildRows();
    // Alarms are checked for the whole life of the window, not only while
    // their page is the visible one.
    m_tick.start();
}

AlarmPage::~AlarmPage() = default;

void AlarmPage::setActive(bool active)
{
    m_active = active;
    if (active)
        checkAlarms();
}

bool AlarmPage::addAlarm(int hour, int minute)
{
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
        return false;
    m_alarms.push_back({hour, minute, true, false});
    rebuildRows();
    return true;
}

bool AlarmPage::removeAlarm(size_t index)
{
    if (index >= m_alarms.size())
        return false;
    m_alarms.erase(m_alarms.begin() + std::ptrdiff_t(index));
    rebuildRows();
    return true;
}

bool AlarmPage::setAlarmEnabled(size_t index, bool enabled)
{
    if (index >= m_alarms.size())
        return false;
    m_alarms[index].enabled = enabled;
    m_alarms[index].fired = false;
    return true;
}

int AlarmPage::alarmHour(size_t index) const
{
    return index < m_alarms.size() ? m_alarms[index].hour : -1;
}

int AlarmPage::alarmMinute(size_t index) const
{
    return index < m_alarms.size() ? m_alarms[index].minute : -1;
}

bool AlarmPage::alarmEnabled(size_t index) const
{
    return index < m_alarms.size() && m_alarms[index].enabled;
}

void AlarmPage::setNowSource(std::function<ClockReading()> source)
{
    m_nowSource = std::move(source);
    checkAlarms();
}

std::string AlarmPage::formatAlarm(int hour, int minute)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
    return buffer;
}

void AlarmPage::addFromInputs()
{
    addAlarm(m_hour->value(), m_minute->value());
}

void AlarmPage::rebuildRows()
{
    while (!m_list->children().empty())
        m_list->takeChild(m_list->children().front().get());
    m_rows.clear();
    for (size_t i = 0; i < m_alarms.size(); ++i) {
        AlarmRow* row = m_list->addChild<AlarmRow>(this, i, m_alarms[i]);
        m_rows.push_back(row);
    }
    updateEmptyState();
    requestLayout();
}

void AlarmPage::updateEmptyState()
{
    m_empty->setText(tr("No alarms yet"));
    m_empty->setVisible(m_alarms.empty());
}

void AlarmPage::checkAlarms()
{
    const ClockReading now = m_nowSource ? m_nowSource() : currentReading();
    const int day = now.year * 10000 + now.month * 100 + now.day;
    if (m_day != day) {
        m_day = day;
        for (Alarm& alarm : m_alarms)
            alarm.fired = false;
    }
    for (Alarm& alarm : m_alarms) {
        if (!alarm.enabled || alarm.fired)
            continue;
        if (alarm.hour != now.hour || alarm.minute != now.minute)
            continue;
        alarm.fired = true;
        if (m_onAlarm)
            m_onAlarm(formatAlarm(alarm.hour, alarm.minute));
    }
}

void AlarmPage::retranslate()
{
    m_addCaption->setText(tr("New alarm"));
    m_add->setText(tr("Add"));
    m_empty->setText(tr("No alarms yet"));
    rebuildRows();
}

SizeF AlarmPage::sizeHint() const
{
    return {760, 520};
}

void AlarmPage::layout()
{
    const RectF rect = localRect();
    const double margin = 30;
    const double controlsHeight = 88;
    m_scroll->setGeometry({margin, 8, std::max(0.0, rect.width - 2 * margin),
                           std::max(0.0, rect.height - controlsHeight - 24)});
    m_empty->setGeometry({margin, 80, std::max(0.0, rect.width - 2 * margin), 32});

    const double controlsY = rect.height - controlsHeight;
    m_addCaption->setGeometry({margin, controlsY, 120, 20});
    m_hour->setGeometry({margin, controlsY + 28, 128, 40});
    m_minute->setGeometry({margin + 144, controlsY + 28, 128, 40});
    m_add->setGeometry({rect.width - margin - 86, controlsY + 30, 86, 36});
}

void AlarmPage::paint(Painter& painter)
{
    const Theme& theme = Theme::current();
    painter.fillRect(localRect(), theme.background);
}

} // namespace Clock

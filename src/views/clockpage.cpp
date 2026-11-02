// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/clockpage.h"

#include "views/analogclock.h"
#include "views/widgets.h"

#include "base/i18n.h"
#include "model/timeformat.h"
#include "ui/color_id.h"
#include "ui/label.h"
#include "ui/painter.h"
#include "ui/theme.h"

#include <algorithm>

using namespace Lingmo::ui;
using namespace std::chrono_literals;
using Lingmo::i18n::tr;

namespace Clock {

ClockPage::ClockPage()
    : m_clock(addChild<AnalogClock>())
    , m_time(addChild<Label>())
    , m_date(addChild<Label>())
    , m_formatButton(addChild<PillButton>(tr("24h"), PillButton::Style::Secondary))
{
    setName("clockPage");
    m_clock->setName("analogClock");

    m_formatButton->setName("formatButton");
    m_formatButton->setCompact(true);
    m_formatButton->setMinimumWidth(58);
    m_formatButton->setFontSize(12.5);
    m_formatButton->onClicked = [this] { toggleTwentyFourHourClock(); };

    m_time->setName("digitalTime");
    m_time->setFontSize(62);
    m_time->setWeight(300);
    m_time->setFontFeatures({{"tnum", 1}});
    m_time->setTextAlignment(Label::TextAlignment::Center);
    m_time->setVerticallyCentered(true);

    m_date->setName("dateLine");
    m_date->setFontSize(15.5);
    m_date->setWeight(400);
    m_date->setColor(ColorRecipe(ColorId::DisabledText));
    m_date->setTextAlignment(Label::TextAlignment::Center);
    m_date->setVerticallyCentered(true);

    m_twentyFourHour = localeUsesTwentyFourHourClock();
    m_formatButton->setText(m_twentyFourHour ? tr("24h") : tr("12h"));
    m_tick.setInterval(40ms);
    m_tick.onTimeout = [this] { update(); };
    update();
}

ClockPage::~ClockPage() = default;

SizeF ClockPage::sizeHint() const
{
    return {760, 520};
}

void ClockPage::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (active) {
        update();
        m_tick.start();
    } else {
        m_tick.stop();
    }
}

void ClockPage::setTimeSource(std::function<ClockReading()> source)
{
    m_timeSource = std::move(source);
    m_shownSecond = -1;
    m_shownDay = -1;
    update();
}

std::string ClockPage::timeText() const
{
    return m_time->text();
}

std::string ClockPage::dateText() const
{
    return m_date->text();
}

void ClockPage::setTwentyFourHourClock(bool twentyFourHour)
{
    if (m_twentyFourHour == twentyFourHour)
        return;
    m_twentyFourHour = twentyFourHour;
    m_formatButton->setText(m_twentyFourHour ? tr("24h") : tr("12h"));
    m_shownSecond = -1;
    update();
}

void ClockPage::toggleTwentyFourHourClock()
{
    setTwentyFourHourClock(!m_twentyFourHour);
}

std::string ClockPage::dateTextFor(const ClockReading& reading) const
{
    std::tm local{};
    local.tm_year = reading.year - 1900;
    local.tm_mon = reading.month - 1;
    local.tm_mday = reading.day;
    local.tm_wday = reading.weekday;
    local.tm_hour = reading.hour;
    local.tm_min = reading.minute;
    local.tm_sec = reading.second;
    return formatLongDate(local);
}

void ClockPage::update()
{
    m_reading = m_timeSource ? m_timeSource() : currentReading();
    m_clock->setReading(m_reading);

    const TimeDigits digits = digitsOf(m_reading);
    if (digits.second != m_shownSecond) {
        m_shownSecond = digits.second;
        m_time->setText(formatClockTime(digits, m_twentyFourHour));
    }
    const int day = m_reading.year * 10000 + m_reading.month * 100 + m_reading.day;
    if (day != m_shownDay) {
        m_shownDay = day;
        m_date->setText(dateTextFor(m_reading));
        requestLayout();
    }
}

void ClockPage::retranslate()
{
    m_formatButton->setText(m_twentyFourHour ? tr("24h") : tr("12h"));
    m_shownSecond = -1;
    m_shownDay = -1;
    update();
}

void ClockPage::layout()
{
    const RectF rect = localRect();
    const double margin = 20;
    const double timeHeight = m_time->sizeHint().height;
    const double dateHeight = m_date->sizeHint().height;
    const double clockSide =
        std::clamp(std::min({rect.width - 2 * margin - 80, rect.height * 0.58, 430.0}), 120.0,
                   std::max(120.0, rect.height));
    const double gapAfterClock = 12;
    const double gapAfterTime = 4;
    const double total = clockSide + gapAfterClock + timeHeight + gapAfterTime + dateHeight;
    double y = std::max(8.0, (rect.height - total) / 2);
    m_formatButton->setGeometry({std::max(margin, rect.width - 86), 10, 66, 28});
    m_clock->setGeometry({(rect.width - clockSide) / 2, y, clockSide, clockSide});
    y += clockSide + gapAfterClock;
    m_time->setGeometry({margin, y, rect.width - 2 * margin, timeHeight});
    y += timeHeight + gapAfterTime;
    m_date->setGeometry({margin, y, rect.width - 2 * margin, dateHeight});
}

void ClockPage::paint(Painter& painter)
{
    const Theme& theme = Theme::current();
    // A soft, wide halo under the dial from the theme: a card the face sits
    // on, so the page has some depth on both the light and the dark theme.
    const RectF rect = localRect();
    if (rect.width < 200 || rect.height < 200)
        return;
    const double haloWidth = std::min(rect.width * 0.62, 520.0);
    const double haloHeight = std::min(rect.height * 0.62, 460.0);
    const RectF halo{(rect.width - haloWidth) / 2, (rect.height - haloHeight) / 2, haloWidth, haloHeight};
    painter.fillRoundedRect(halo, std::min(haloWidth, haloHeight) / 2,
                            theme.darkMode ? theme.alternateBackground.withAlpha(0.28)
                                           : theme.text.withAlpha(0.022));
}

} // namespace Clock

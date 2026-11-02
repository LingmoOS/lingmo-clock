// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "views/worldclockpage.h"

#include "views/widgets.h"

#include "base/i18n.h"
#include "model/timeformat.h"
#include "ui/color_id.h"
#include "ui/combo_box.h"
#include "ui/label.h"
#include "ui/layout.h"
#include "ui/painter.h"
#include "ui/scroll_view.h"
#include "ui/text_layout.h"
#include "ui/theme.h"

#include <algorithm>
#include <cstdio>

using namespace Lingmo::ui;
using namespace std::chrono_literals;
using Lingmo::i18n::tr;

namespace Clock {

namespace {

constexpr double kMargin = 30;
constexpr double kRowHeight = 84;
constexpr double kRowSpacing = 10;
constexpr double kBottomRowHeight = 40;

std::string shortTimeText(const std::tm& local)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", local.tm_hour, local.tm_min);
    return buffer;
}

std::tm momentToTm(const LocalMoment& moment)
{
    std::tm local{};
    local.tm_year = moment.year - 1900;
    local.tm_mon = moment.month - 1;
    local.tm_mday = moment.day;
    local.tm_wday = moment.weekday;
    local.tm_hour = moment.hour;
    local.tm_min = moment.minute;
    local.tm_sec = moment.second;
    return local;
}

} // namespace

// One city's card: its name and offset on the left, its time on the right.
class WorldClockPage::CityRow : public Lingmo::ui::View
{
public:
    explicit CityRow(WorldClockPage* page, std::string zone, std::string name)
        : m_page(page)
        , m_zone(std::move(zone))
        , m_name(addChild<Label>(std::move(name)))
        , m_caption(addChild<Label>())
        , m_time(addChild<Label>())
        , m_remove(addChild<RoundButton>(RoundButton::Glyph::Close, 28))
    {
        setName("cityRow");
        m_name->setName("cityName");
        m_name->setFontSize(17);
        m_name->setWeight(600);
        m_name->setVerticallyCentered(true);
        m_name->setElide(Label::Elide::Right);

        m_caption->setName("cityCaption");
        m_caption->setFontSize(12);
        m_caption->setWeight(400);
        m_caption->setColor(ColorRecipe(ColorId::DisabledText));
        m_caption->setVerticallyCentered(true);
        m_caption->setElide(Label::Elide::Right);

        m_time->setName("cityTime");
        m_time->setFontFamily(systemFixedFont().family);
        m_time->setFontSize(30);
        m_time->setWeight(400);
        m_time->setTextAlignment(Label::TextAlignment::Right);
        m_time->setVerticallyCentered(true);

        m_remove->setName("removeCityButton");
        m_remove->onClicked = [this] { m_page->removeCity(index()); };
    }

    const std::string& zone() const { return m_zone; }
    std::string timeText() const { return m_time->text(); }
    std::string captionText() const { return m_caption->text(); }
    Label* nameLabel() const { return m_name; }

    size_t index() const
    {
        const auto& zones = m_page->m_clock.zones();
        for (size_t i = 0; i < zones.size(); ++i) {
            if (zones[i] == m_zone)
                return i;
        }
        return 0;
    }

    void update(std::time_t now, const LocalMoment& reference)
    {
        const int offset = m_page->m_clock.offsetAt(m_zone, now);
        const LocalMoment moment = WorldClock::momentAtOffset(now, offset);
        m_time->setText(shortTimeText(momentToTm(moment)));
        std::string caption = formatUtcOffset(offset);
        const int days = WorldClock::dayDifference(moment, reference);
        if (days > 0)
            caption += std::string(" · ") + tr("Tomorrow");
        else if (days < 0)
            caption += std::string(" · ") + tr("Yesterday");
        if (moment.hour < 6 || moment.hour >= 18)
            m_time->setColor(ColorRecipe(ColorId::DisabledText));
        else
            m_time->setColor(std::nullopt);
        m_caption->setText(caption);
    }

    SizeF sizeHint() const override { return {0, kRowHeight}; }

protected:
    void layout() override
    {
        const RectF rect = localRect();
        const double removeSize = 28;
        const double timeWidth = 132;
        const double right = rect.width - 18 - removeSize - 14;
        m_name->setGeometry({20, 15, std::max(0.0, right - timeWidth - 20 - 16), 24});
        m_caption->setGeometry({20, 45, std::max(0.0, right - timeWidth - 20 - 16), 18});
        m_time->setGeometry({right - timeWidth, 18, timeWidth, 38});
        m_remove->setGeometry({rect.width - 18 - removeSize, (rect.height - removeSize) / 2, removeSize,
                               removeSize});
    }

    void paint(Painter& painter) override
    {
        const Theme& theme = Theme::current();
        const Corners corners = theme.continuousCorners ? Corners::Continuous : Corners::Circular;
        const RectF rect = localRect();
        painter.drawShadow(rect, theme.bigRadius, 8, Color{0, 0, 0, uint8_t(theme.darkMode ? 60 : 14)},
                           {0, 2});
        painter.fillRoundedRect(rect, theme.bigRadius, theme.secondBackground, corners);
        painter.strokeRoundedRect(rect, theme.bigRadius, theme.text.withAlpha(0.05),
                                  painter.borderWidth(1), corners);
    }

private:
    WorldClockPage* m_page;
    std::string m_zone;
    Label* m_name;
    Label* m_caption;
    Label* m_time;
    RoundButton* m_remove;
};

const std::vector<WorldClockPage::City>& WorldClockPage::knownCities()
{
    static const std::vector<City> cities{
        {"UTC", "UTC"},
        {"Europe/London", "London"},
        {"Europe/Paris", "Paris"},
        {"Europe/Berlin", "Berlin"},
        {"Europe/Moscow", "Moscow"},
        {"America/New_York", "New York"},
        {"America/Chicago", "Chicago"},
        {"America/Denver", "Denver"},
        {"America/Los_Angeles", "Los Angeles"},
        {"America/Sao_Paulo", "Sao Paulo"},
        {"Asia/Dubai", "Dubai"},
        {"Asia/Kolkata", "New Delhi"},
        {"Asia/Shanghai", "Shanghai"},
        {"Asia/Singapore", "Singapore"},
        {"Asia/Tokyo", "Tokyo"},
        {"Asia/Seoul", "Seoul"},
        {"Australia/Sydney", "Sydney"},
        {"Pacific/Auckland", "Auckland"},
    };
    return cities;
}

WorldClockPage::WorldClockPage()
    : m_scroll(addChild<ScrollView>())
    , m_list(nullptr)
    , m_empty(addChild<Label>())
    , m_addBox(addChild<ComboBox>())
    , m_addButton(addChild<PillButton>(tr("Add"), PillButton::Style::Accent))
    , m_addCaption(addChild<Label>())
{
    setName("worldClockPage");
    m_scroll->setName("citiesScroll");
    m_scroll->setVerticalBarPadding(4, 4);
    m_scroll->setElastic(false);
    auto column = std::make_unique<Column>();
    column->setSpacing(kRowSpacing);
    column->setName("cityList");
    m_list = column.get();
    m_scroll->setContentView(std::move(column));

    m_empty->setName("emptyCities");
    m_empty->setFontSize(14);
    m_empty->setColor(ColorRecipe(ColorId::DisabledText));
    m_empty->setTextAlignment(Label::TextAlignment::Center);
    m_empty->setVerticallyCentered(true);
    m_empty->setVisible(false);

    m_addBox->setName("addCityBox");
    m_addBox->onActivated = [this](int) { addSelectedCity(); };
    m_addButton->setName("addCityButton");
    m_addButton->setCompact(true);
    m_addButton->onClicked = [this] { addSelectedCity(); };

    m_addCaption->setFontSize(13);
    m_addCaption->setWeight(500);
    m_addCaption->setColor(ColorRecipe(ColorId::DisabledText));
    m_addCaption->setVerticallyCentered(true);

    m_clock.addZone("Europe/London");
    m_clock.addZone("America/New_York");
    m_clock.addZone("Asia/Shanghai");
    m_clock.addZone("Asia/Tokyo");

    m_tick.setInterval(1s);
    m_tick.onTimeout = [this] { refresh(); };

    retranslate();
    rebuildRows();
}

WorldClockPage::~WorldClockPage() = default;

SizeF WorldClockPage::sizeHint() const
{
    return {760, 520};
}

void WorldClockPage::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (active) {
        refresh();
        m_tick.start();
    } else {
        m_tick.stop();
    }
}

std::string WorldClockPage::nameOf(const std::string& zone) const
{
    for (const City& city : knownCities()) {
        if (city.zone == zone)
            return city.name;
    }
    return zone;
}

std::string WorldClockPage::availableCityName(size_t index) const
{
    if (index >= m_availableZones.size())
        return {};
    return tr(nameOf(m_availableZones[index]).c_str());
}

std::string WorldClockPage::cityZone(size_t index) const
{
    const auto& zones = m_clock.zones();
    return index < zones.size() ? zones[index] : std::string();
}

std::string WorldClockPage::cityName(size_t index) const
{
    const auto& zones = m_clock.zones();
    return index < zones.size() ? tr(nameOf(zones[index]).c_str()) : std::string();
}

std::string WorldClockPage::cityTimeText(size_t index) const
{
    return index < m_rows.size() ? m_rows[index]->timeText() : std::string();
}

std::string WorldClockPage::cityCaptionText(size_t index) const
{
    return index < m_rows.size() ? m_rows[index]->captionText() : std::string();
}

void WorldClockPage::setNowSource(std::function<std::time_t()> source)
{
    m_nowSource = std::move(source);
    refresh();
}

bool WorldClockPage::addCity(const std::string& zone)
{
    if (!m_clock.addZone(zone))
        return false;
    rebuildRows();
    refreshAvailableCities();
    return true;
}

bool WorldClockPage::removeCity(size_t index)
{
    if (!m_clock.removeZone(index))
        return false;
    rebuildRows();
    refreshAvailableCities();
    return true;
}

void WorldClockPage::addSelectedCity()
{
    const int current = m_addBox->currentIndex();
    if (current < 0 || size_t(current) >= m_availableZones.size())
        return;
    addCity(m_availableZones[size_t(current)]);
}

void WorldClockPage::refreshAvailableCities()
{
    m_availableZones.clear();
    std::vector<std::string> items;
    for (const City& city : knownCities()) {
        const bool present = std::find(m_clock.zones().begin(), m_clock.zones().end(), city.zone)
            != m_clock.zones().end();
        if (present)
            continue;
        m_availableZones.push_back(city.zone);
        items.push_back(tr(city.name.c_str()));
    }
    m_addBox->setItems(items);
    if (!items.empty())
        m_addBox->setCurrentIndex(0);
    m_addBox->setEnabled(!items.empty());
    m_addButton->setEnabled(!items.empty());
    m_addBox->setVisible(!items.empty());
    m_addCaption->setVisible(!items.empty());
    m_addBox->requestLayout();
}

void WorldClockPage::rebuildRows()
{
    for (CityRow* row : m_rows)
        m_list->takeChild(row);
    m_rows.clear();
    for (const std::string& zone : m_clock.zones()) {
        auto* row = m_list->addChild<CityRow>(this, zone, tr(nameOf(zone).c_str()));
        row->setLayoutHints({.fillWidth = true, .preferredHeight = kRowHeight});
        m_rows.push_back(row);
    }
    m_empty->setVisible(m_rows.empty());
    m_list->requestLayout();
    refresh();
    refreshAvailableCities();
}

void WorldClockPage::refresh()
{
    const std::time_t now = m_nowSource ? m_nowSource() : std::time(nullptr);
    std::tm base{};
    localtime_r(&now, &base);
    LocalMoment reference;
    reference.year = base.tm_year + 1900;
    reference.month = base.tm_mon + 1;
    reference.day = base.tm_mday;
    reference.weekday = base.tm_wday;
    reference.hour = base.tm_hour;
    reference.minute = base.tm_min;
    reference.second = base.tm_sec;
    for (CityRow* row : m_rows)
        row->update(now, reference);
}

void WorldClockPage::retranslate()
{
    m_addCaption->setText(tr("Add city"));
    m_addButton->setText(tr("Add"));
    m_empty->setText(tr("No cities yet."));
    // City names and the add box's items take the language again.
    for (CityRow* row : m_rows)
        row->nameLabel()->setText(tr(nameOf(row->zone()).c_str()));
    refreshAvailableCities();
    if (m_rows.empty()) {
        rebuildRows();
    } else {
        refresh();
    }
    requestLayout();
}

void WorldClockPage::layout()
{
    const RectF rect = localRect();
    const double bottomRowTop = rect.height - kBottomRowHeight;
    m_scroll->setGeometry({kMargin, 10, std::max(0.0, rect.width - 2 * kMargin),
                           std::max(0.0, bottomRowTop - 10 - 12)});
    m_empty->setGeometry({kMargin, 10, std::max(0.0, rect.width - 2 * kMargin),
                          std::max(0.0, bottomRowTop - 10 - 12)});

    const double captionWidth = 74;
    const double comboWidth = std::min(280.0, rect.width - 2 * kMargin - captionWidth - 100);
    const double y = bottomRowTop + (kBottomRowHeight - 40) / 2;
    m_addCaption->setGeometry({kMargin, y + 11, captionWidth, 18});
    m_addBox->setGeometry({kMargin + captionWidth, y, std::max(120.0, comboWidth), 40});
    m_addButton->setGeometry({kMargin + captionWidth + comboWidth + 12, y, 92, 40});
}

void WorldClockPage::paint(Painter& painter)
{
    (void)painter;
}

} // namespace Clock

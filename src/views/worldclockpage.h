// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

#include "base/timer.h"
#include "model/worldclock.h"
#include "ui/geometry.h"
#include "ui/view.h"

namespace Lingmo::ui {
class Column;
class ComboBox;
class Label;
class ScrollView;
} // namespace Lingmo::ui

namespace Clock {

class PillButton;

// The world clock: a few cities, each with its offset and local time, and a
// way to add and remove them.
class WorldClockPage : public Lingmo::ui::View
{
public:
    WorldClockPage();
    ~WorldClockPage() override;

    void setActive(bool active);
    bool isActive() const { return m_active; }

    // A city is a zone's identifier, an IANA name.
    bool addCity(const std::string& zone);
    bool removeCity(size_t index);
    // Every city there is to add, in the box's order.
    size_t availableCityCount() const { return m_availableZones.size(); }
    std::string availableCityName(size_t index) const;

    size_t cityCount() const { return m_clock.zones().size(); }
    std::string cityZone(size_t index) const;
    std::string cityName(size_t index) const;
    std::string cityTimeText(size_t index) const;
    std::string cityCaptionText(size_t index) const;

    // For tests: reads the time from here instead of the system.
    void setNowSource(std::function<std::time_t()> source);

    void retranslate();
    void refresh();

    Lingmo::ui::SizeF sizeHint() const override;

protected:
    void layout() override;
    void paint(Lingmo::ui::Painter& painter) override;

private:
    class CityRow;

    struct City
    {
        std::string zone;
        // The source name, translated where it is shown.
        std::string name;
    };

    static const std::vector<City>& knownCities();
    std::string nameOf(const std::string& zone) const;
    void rebuildRows();
    void refreshAvailableCities();
    void addSelectedCity();

    WorldClock m_clock;
    Lingmo::ui::ScrollView* m_scroll;
    Lingmo::ui::Column* m_list;
    Lingmo::ui::Label* m_empty;
    Lingmo::ui::ComboBox* m_addBox;
    PillButton* m_addButton;
    Lingmo::ui::Label* m_addCaption;
    std::vector<CityRow*> m_rows;
    std::vector<std::string> m_availableZones;
    std::function<std::time_t()> m_nowSource;
    Lingmo::base::Timer m_tick;
    bool m_active = false;
};

} // namespace Clock

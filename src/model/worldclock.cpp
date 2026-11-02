// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "model/worldclock.h"

#include <cstdlib>
#include <cstring>

namespace Clock {

int systemUtcOffset(const std::string& zone, std::time_t utc)
{
    if (zone.empty())
        return 0;
    // localtime_r reads the TZ environment: set the zone, read the offset the
    // C library worked out for that instant, then put the environment back.
    const char* previous = std::getenv("TZ");
    const std::string saved = previous ? previous : std::string();
    setenv("TZ", zone.c_str(), 1);
    tzset();
    std::tm local{};
    int offset = 0;
    if (localtime_r(&utc, &local) != nullptr)
        offset = int(local.tm_gmtoff);
    if (previous)
        setenv("TZ", saved.c_str(), 1);
    else
        unsetenv("TZ");
    tzset();
    return offset;
}

WorldClock::WorldClock(OffsetFunction offset)
    : m_offset(std::move(offset))
{
}

bool WorldClock::addZone(const std::string& zone)
{
    if (zone.empty())
        return false;
    for (const std::string& existing : m_zones) {
        if (existing == zone)
            return false;
    }
    m_zones.push_back(zone);
    return true;
}

bool WorldClock::removeZone(size_t index)
{
    if (index >= m_zones.size())
        return false;
    m_zones.erase(m_zones.begin() + std::ptrdiff_t(index));
    return true;
}

int WorldClock::offsetAt(const std::string& zone, std::time_t utc) const
{
    if (m_offset)
        return m_offset(zone, utc);
    return systemUtcOffset(zone, utc);
}

LocalMoment WorldClock::moment(const std::string& zone, std::time_t utc) const
{
    return momentAtOffset(utc, offsetAt(zone, utc));
}

LocalMoment WorldClock::momentAtOffset(std::time_t utc, int offsetSeconds)
{
    const std::time_t shifted = utc + std::time_t(offsetSeconds);
    std::tm local{};
    gmtime_r(&shifted, &local);
    LocalMoment moment;
    moment.year = local.tm_year + 1900;
    moment.month = local.tm_mon + 1;
    moment.day = local.tm_mday;
    moment.weekday = local.tm_wday;
    moment.hour = local.tm_hour;
    moment.minute = local.tm_min;
    moment.second = local.tm_sec;
    return moment;
}

int WorldClock::dayDifference(const LocalMoment& zone, const LocalMoment& reference)
{
    const int zoned = zone.year * 10000 + zone.month * 100 + zone.day;
    const int referenceDay = reference.year * 10000 + reference.month * 100 + reference.day;
    if (zoned < referenceDay)
        return -1;
    if (zoned > referenceDay)
        return 1;
    return 0;
}

} // namespace Clock

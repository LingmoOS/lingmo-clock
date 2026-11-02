// SPDX-FileCopyrightText: 2026 LingmoOS Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace Clock {

// A local date and time in some time zone.
struct LocalMoment
{
    int year = 1970;
    int month = 1;
    int day = 1;
    // 0 is Sunday.
    int weekday = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    bool operator==(const LocalMoment&) const = default;
};

// The offset of an IANA zone from UTC at an instant, in seconds, as the
// system's zone database knows it; the TZ environment is restored after.
int systemUtcOffset(const std::string& zone, std::time_t utc);

// The cities one window shows. The offset lookup is a parameter so tests can
// fix it themselves.
class WorldClock
{
public:
    using OffsetFunction = std::function<int(const std::string& zone, std::time_t utc)>;

    explicit WorldClock(OffsetFunction offset = {});

    // False for an empty or duplicate zone.
    bool addZone(const std::string& zone);
    bool removeZone(size_t index);
    const std::vector<std::string>& zones() const { return m_zones; }

    int offsetAt(const std::string& zone, std::time_t utc) const;
    LocalMoment moment(const std::string& zone, std::time_t utc) const;

    static LocalMoment momentAtOffset(std::time_t utc, int offsetSeconds);
    // -1, 0 or 1: the zone's date against the reference's, for "Yesterday"
    // and "Tomorrow".
    static int dayDifference(const LocalMoment& zone, const LocalMoment& reference);

private:
    OffsetFunction m_offset;
    std::vector<std::string> m_zones;
};

} // namespace Clock

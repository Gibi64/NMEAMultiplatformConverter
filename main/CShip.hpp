#pragma once

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "CTimeUtils.hpp"

class CShip
{
public:
    struct sUTCTime
    {
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int millisecond = 0;

        bool empty() const
        {
            return year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && millisecond == 0;
        }
    };

    static std::string CalculateNMEAChecksum(const std::string& sentence)
    {
        uint8_t checksum = 0;
        size_t start = 0;
        if (!sentence.empty() && (sentence[0] == '$' || sentence[0] == '!'))
            start = 1;

        for (size_t i = start; i < sentence.size(); ++i)
            checksum ^= static_cast<uint8_t>(sentence[i]);

        char buf[8];
        snprintf(buf, sizeof(buf), "*%02X\r\n", checksum);
        return sentence + buf;
    }

    static std::string ToNMEA0183Coord(double deg, bool isLat)
    {
        char hemi = (isLat ? (deg >= 0 ? 'N' : 'S') : (deg >= 0 ? 'E' : 'W'));
        deg = std::fabs(deg);

        int d = static_cast<int>(deg);
        double m = (deg - d) * 60.0;

        char buf[32];
        snprintf(buf, sizeof(buf), isLat ? "%02d%07.4f,%c" : "%03d%07.4f,%c", d, m, hemi);
        return std::string(buf);
    }

protected:
    struct sKinematics
    {
        bool valid = false;
        double sogKnots = std::numeric_limits<double>::quiet_NaN();
        double cogDeg = std::numeric_limits<double>::quiet_NaN();
    };

    CShip()
    {
        m_SogKnots = std::numeric_limits<double>::quiet_NaN();
        m_CogDeg = std::numeric_limits<double>::quiet_NaN();
    }

    void SetSogCog(double sogKnots, double cogDeg)
    {
        m_SogKnots = sogKnots;
        m_CogDeg = NormalizeCourse(cogDeg);
    }

    double GetSogKnots() const { return m_SogKnots; }
    double GetCogDeg() const { return m_CogDeg; }

    static sKinematics ComputeSogCog(double prevLatDeg, double prevLonDeg, uint64_t prevMs, double latDeg, double lonDeg, uint64_t nowMs)
    {
        sKinematics out;
        if (nowMs <= prevMs)
            return out;

        constexpr double PI = 3.14159265358979323846;
        auto deg2rad = [](double d) { return d * PI / 180.0; };

        const double lat1 = deg2rad(prevLatDeg);
        const double lon1 = deg2rad(prevLonDeg);
        const double lat2 = deg2rad(latDeg);
        const double lon2 = deg2rad(lonDeg);

        const double dLat = lat2 - lat1;
        const double dLon = lon2 - lon1;
        const double latMean = (lat1 + lat2) / 2.0;

        constexpr double R = 6371000.0;
        const double dNorth = R * dLat;
        const double dEast = R * cos(latMean) * dLon;

        const double distanceMeters = sqrt(dNorth * dNorth + dEast * dEast);
        const double dtSeconds = static_cast<double>(nowMs - prevMs) / 1000.0;
        if (dtSeconds <= 0.0)
            return out;

        const double speedMS = distanceMeters / dtSeconds;
        out.sogKnots = speedMS / 0.514444;

        double course = atan2(dEast, dNorth) * 180.0 / PI;
        if (course < 0.0)
            course += 360.0;
        out.cogDeg = course;
        out.valid = true;
        return out;
    }

    static std::string BuildAisAivdmType1(uint32_t mmsi, double latitude, double longitude, double sogKnots, double cogDeg)
    {
        std::vector<bool> bits;
        bits.reserve(168);

        const bool lonValid = (longitude >= -180.0 && longitude <= 180.0);
        const bool latValid = (latitude >= -90.0 && latitude <= 90.0);
        const bool sogValid = std::isfinite(sogKnots) && sogKnots >= 0.0;
        const bool cogValid = std::isfinite(cogDeg) && cogDeg >= 0.0;

        int32_t rawLon = lonValid ? static_cast<int32_t>(llround(longitude * 600000.0)) : 0x06791AC0;
        int32_t rawLat = latValid ? static_cast<int32_t>(llround(latitude * 600000.0)) : 0x03412140;
        uint16_t rawSog = 1023U;
        if (sogValid)
        {
            const double boundedSog = std::min(sogKnots, 102.2);
            rawSog = static_cast<uint16_t>(llround(boundedSog * 10.0));
        }

        uint16_t rawCog = 3600U;
        uint16_t rawHeading = 511U;
        if (cogValid)
        {
            const double boundedCog = NormalizeCourse(cogDeg);
            rawCog = static_cast<uint16_t>(llround(boundedCog * 10.0));
            rawHeading = static_cast<uint16_t>(llround(boundedCog));
            if (rawHeading > 359U)
                rawHeading = 359U;
        }

        AppendBits(bits, 1U, 6);
        AppendBits(bits, 0U, 2);
        AppendBits(bits, mmsi, 30);
        AppendBits(bits, 0U, 4);
        AppendBits(bits, 128U, 8);
        AppendBits(bits, rawSog, 10);
        AppendBits(bits, 0U, 1);
        AppendSignedBits(bits, rawLon, 28);
        AppendSignedBits(bits, rawLat, 27);
        AppendBits(bits, rawCog, 12);
        AppendBits(bits, rawHeading, 9);
        AppendBits(bits, 60U, 6);
        AppendBits(bits, 0U, 2);
        AppendBits(bits, 0U, 3);
        AppendBits(bits, 0U, 1);
        AppendBits(bits, 0U, 19);

        std::string payload;
        payload.reserve(bits.size() / 6);
        for (size_t i = 0; i < bits.size(); i += 6)
        {
            uint8_t value = 0;
            for (size_t b = 0; b < 6; ++b)
            {
                value <<= 1;
                value |= bits[i + b] ? 1U : 0U;
            }
            payload.push_back(ToAis6BitChar(value));
        }

        return CalculateNMEAChecksum("!AIVDM,1,1,,A," + payload + ",0");
    }

private:
    static double NormalizeCourse(double course)
    {
        if (!std::isfinite(course))
            return 0.0;

        while (course < 0.0)
            course += 360.0;
        while (course >= 360.0)
            course -= 360.0;
        return course;
    }

private:
    double m_SogKnots;
    double m_CogDeg;

    static void AppendBits(std::vector<bool>& bits, uint32_t value, int bitCount)
    {
        for (int i = bitCount - 1; i >= 0; --i)
            bits.push_back(((value >> i) & 0x1U) != 0);
    }

    static void AppendSignedBits(std::vector<bool>& bits, int32_t value, int bitCount)
    {
        const int64_t limit = (static_cast<int64_t>(1) << bitCount);
        int64_t raw = value;
        if (raw < 0)
            raw = limit + raw;
        AppendBits(bits, static_cast<uint32_t>(raw), bitCount);
    }

    static char ToAis6BitChar(uint8_t value)
    {
        value &= 0x3F;
        return static_cast<char>(value < 40 ? (value + 48) : (value + 56));
    }
};

class CMyShip : public CShip
{
public:
    bool DecodePGN129025(const std::vector<unsigned char>& encoded)
    {
        if (encoded.size() < 8 || m_UTCTime.empty())
            return false;

        int32_t rawLat = static_cast<int32_t>(encoded[0]) |
            (static_cast<int32_t>(encoded[1]) << 8) |
            (static_cast<int32_t>(encoded[2]) << 16) |
            (static_cast<int32_t>(encoded[3]) << 24);

        int32_t rawLon = static_cast<int32_t>(encoded[4]) |
            (static_cast<int32_t>(encoded[5]) << 8) |
            (static_cast<int32_t>(encoded[6]) << 16) |
            (static_cast<int32_t>(encoded[7]) << 24);

        sRMCData rmcData;
        rmcData.utcTime = m_UTCTime;
        rmcData.latitude = static_cast<double>(rawLat) / 1e7;
        rmcData.longitude = static_cast<double>(rawLon) / 1e7;

        return StackMeanNavData(rmcData);
    }

    bool DecodePGN126992(const std::vector<unsigned char>& encoded)
    {
        if (encoded.size() < 8)
            return false;

        uint32_t timeMs =
            static_cast<uint32_t>(encoded[0]) |
            (static_cast<uint32_t>(encoded[1]) << 8) |
            (static_cast<uint32_t>(encoded[2]) << 16) |
            (static_cast<uint32_t>(encoded[3]) << 24);

        uint32_t dateDays =
            static_cast<uint32_t>(encoded[4]) |
            (static_cast<uint32_t>(encoded[5]) << 8);

        double seconds = timeMs / 1000.0;
        int hours = static_cast<int>(seconds / 3600);
        int minutes = static_cast<int>((seconds - hours * 3600) / 60);
        int secs = static_cast<int>(seconds) % 60;
        int milliseconds = static_cast<int>(timeMs % 1000);
        CTimeUtils::sDate s = CTimeUtils::SystemDate(dateDays);

        m_UTCTime.day = s.day;
        m_UTCTime.month = s.month;
        m_UTCTime.year = s.year;
        m_UTCTime.hour = hours;
        m_UTCTime.minute = minutes;
        m_UTCTime.second = secs;
        m_UTCTime.millisecond = milliseconds;
        return true;
    }

    bool DecodePGN128267(const std::vector<unsigned char>& encoded)
    {
        if (encoded.size() < 8)
            return false;

        uint16_t rawDepth =
            static_cast<uint16_t>(encoded[0]) |
            (static_cast<uint16_t>(encoded[1]) << 8);

        return StackMeanDepthData(rawDepth);
    }

    bool DecodePGN130306(const std::vector<unsigned char>& encoded)
    {
        if (encoded.size() < 6)
            return false;

        uint16_t rawAngle =
            static_cast<uint16_t>(encoded[1]) |
            (static_cast<uint16_t>(encoded[2]) << 8);

        uint16_t rawSpeed =
            static_cast<uint16_t>(encoded[3]) |
            (static_cast<uint16_t>(encoded[4]) << 8);

        return StackMeanWindData(rawSpeed, rawAngle);
    }

    bool DecodePGN128259(const std::vector<unsigned char>& encoded)
    {
        return encoded.size() >= 2;
    }

    bool DecodePGN127250(const std::vector<unsigned char>& encoded)
    {
        return encoded.size() >= 2;
    }

    std::string BuildRMC()
    {
        if (m_RMCMeanData.m_Count <= 0)
            return "";

        const auto& t = m_RMCMeanData.RMCMeanData.utcTime;

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf),
            "%02d%02d%02d.%03d",
            t.hour, t.minute, t.second, t.millisecond);

        int yy = t.year % 100;
        char dateBuf[8];
        snprintf(dateBuf, sizeof(dateBuf),
            "%02d%02d%02d",
            t.day, t.month, yy);

        std::string latStr = ToNMEA0183Coord(m_RMCMeanData.RMCMeanData.latitude, true);
        std::string lonStr = ToNMEA0183Coord(m_RMCMeanData.RMCMeanData.longitude, false);

        auto splitCoord = [](const std::string& s)
            {
                auto commaPos = s.find(',');
                return std::make_pair(s.substr(0, commaPos), s.substr(commaPos + 1));
            };

        auto lat = splitCoord(latStr);
        auto lon = splitCoord(lonStr);

        char buf[128];
        snprintf(buf, sizeof(buf),
            "$GPRMC,%s,A,%s,%s,%s,%s,%.2f,%.2f,%s,,,A",
            timeBuf,
            lat.first.c_str(), lat.second.c_str(),
            lon.first.c_str(), lon.second.c_str(),
            m_RMCMeanData.RMCMeanData.speedKnots,
            m_RMCMeanData.RMCMeanData.courseOverGround,
            dateBuf);

        auto result = CalculateNMEAChecksum(std::string(buf));
        m_RMCMeanData.m_Count = 0;
        return result;
    }

    std::string BuildDBT()
    {
        if (m_RMCMeanData.m_Count <= 0)
            return "";

        double depthMeters = m_RMCMeanData.RMCMeanData.speedKnots;
        char buf[64];
        snprintf(buf, sizeof(buf), "$SDDBT,%.1f,f,%.1f,M,%.1f,F", depthMeters * 3.28084, depthMeters, depthMeters * 3.28084);
        return CalculateNMEAChecksum(std::string(buf));
    }

    std::string BuildMWV()
    {
        if (m_RMCMeanData.m_Count <= 0)
            return "";

        double windSpeed = m_RMCMeanData.RMCMeanData.speedKnots;
        double windAngle = m_RMCMeanData.RMCMeanData.courseOverGround;
        char buf[64];
        snprintf(buf, sizeof(buf), "$WIMWV,%.1f,R,%.1f,N,A", windAngle, windSpeed);
        return CalculateNMEAChecksum(std::string(buf));
    }

private:
    struct sRMCData
    {
        double latitude = 0.0;
        double longitude = 0.0;
        double speedKnots = 0.0;
        double courseOverGround = 0.0;
        sUTCTime utcTime;
    };

    struct sNavDelta
    {
        double distanceMeters;
        double speedKnots;
        double courseDegrees;
    };

    struct sRMCMeanData
    {
        int m_Count = 0;
        sRMCData RMCMeanData;
    };

    sUTCTime m_UTCTime;
    sRMCMeanData m_RMCMeanData;

    static uint64_t ToEpoch2001ms(const sUTCTime& t)
    {
        static const int mdays[12] =
        { 31,28,31,30,31,30,31,31,30,31,30,31 };

        int y = t.year;
        int m = t.month;
        int d = t.day;

        uint64_t days = 0;
        for (int year = 2001; year < y; ++year)
        {
            days += ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
                ? 366 : 365;
        }

        for (int month = 1; month < m; ++month)
        {
            days += mdays[month - 1];
            if (month == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
                days += 1;
        }

        days += (d - 1);

        uint64_t ms = days * 24ULL * 3600ULL * 1000ULL;
        ms += t.hour * 3600ULL * 1000ULL;
        ms += t.minute * 60ULL * 1000ULL;
        ms += t.second * 1000ULL;
        ms += t.millisecond;

        return ms;
    }

    static sNavDelta ComputeDelta(const sRMCData& p1, const sRMCData& p2)
    {
        constexpr double PI = 3.14159265358979323846;
        auto deg2rad = [](double d) { return d * PI / 180.0; };

        double lat1 = deg2rad(p1.latitude);
        double lon1 = deg2rad(p1.longitude);
        double lat2 = deg2rad(p2.latitude);
        double lon2 = deg2rad(p2.longitude);

        double dLat = lat2 - lat1;
        double dLon = lon2 - lon1;
        double latMean = (lat1 + lat2) / 2.0;

        constexpr double R = 6371000.0;
        double dNorth = R * dLat;
        double dEast = R * cos(latMean) * dLon;

        double distance = sqrt(dNorth * dNorth + dEast * dEast);

        uint64_t t1 = ToEpoch2001ms(p1.utcTime);
        uint64_t t2 = ToEpoch2001ms(p2.utcTime);

        double dt = static_cast<double>(t2 - t1) / 1000.0;
        if (dt <= 0.0)
            dt = 1.0;

        double speedMS = distance / dt;
        double speedKnots = speedMS / 0.514444;

        double courseRad = atan2(dEast, dNorth);
        double courseDeg = courseRad * 180.0 / PI;
        if (courseDeg < 0)
            courseDeg += 360.0;

        return { distance, speedKnots, courseDeg };
    }

    bool StackMeanNavData(const sRMCData& data)
    {
        if (!m_RMCMeanData.m_Count)
        {
            sRMCData seed = data;
            seed.speedKnots = 0;
            seed.courseOverGround = 0;
            m_RMCMeanData.RMCMeanData = seed;
            m_RMCMeanData.m_Count = 1;
            return false;
        }

        auto& p1 = m_RMCMeanData.RMCMeanData;
        sNavDelta delta = ComputeDelta(p1, data);
        double meanSpeed = (m_RMCMeanData.m_Count * p1.speedKnots + delta.speedKnots)
            / (m_RMCMeanData.m_Count + 1);
        double meanCourse = (m_RMCMeanData.m_Count * p1.courseOverGround + delta.courseDegrees)
            / (m_RMCMeanData.m_Count + 1);

        m_RMCMeanData.m_Count++;
        p1.latitude = data.latitude;
        p1.longitude = data.longitude;
        p1.speedKnots = meanSpeed;
        p1.courseOverGround = meanCourse;
        p1.utcTime = data.utcTime;
        SetSogCog(meanSpeed, meanCourse);
        return true;
    }

    bool StackMeanDepthData(double depth)
    {
        if (!m_RMCMeanData.m_Count)
        {
            m_RMCMeanData.RMCMeanData.speedKnots = 0;
            m_RMCMeanData.RMCMeanData.courseOverGround = 0;
            m_RMCMeanData.m_Count = 1;
            return false;
        }

        m_RMCMeanData.RMCMeanData.speedKnots =
            (m_RMCMeanData.m_Count * m_RMCMeanData.RMCMeanData.speedKnots + depth)
            / (m_RMCMeanData.m_Count + 1);
        m_RMCMeanData.m_Count++;
        return true;
    }

    bool StackMeanWindData(double speed, double angle)
    {
        if (!m_RMCMeanData.m_Count)
        {
            m_RMCMeanData.RMCMeanData.speedKnots = 0;
            m_RMCMeanData.RMCMeanData.courseOverGround = 0;
            m_RMCMeanData.m_Count = 1;
            return false;
        }

        m_RMCMeanData.RMCMeanData.speedKnots =
            (m_RMCMeanData.m_Count * m_RMCMeanData.RMCMeanData.speedKnots + speed)
            / (m_RMCMeanData.m_Count + 1);
        m_RMCMeanData.RMCMeanData.courseOverGround =
            (m_RMCMeanData.m_Count * m_RMCMeanData.RMCMeanData.courseOverGround + angle)
            / (m_RMCMeanData.m_Count + 1);
        m_RMCMeanData.m_Count++;
        return true;
    }
};

class COtherBoats : public CShip
{
public:
    std::string DecodePGN129038(const std::vector<unsigned char>& encoded)
    {
        if (encoded.size() < 13)
            return {};

        uint32_t mmsi = static_cast<uint32_t>(encoded[1]) |
            (static_cast<uint32_t>(encoded[2]) << 8) |
            (static_cast<uint32_t>(encoded[3]) << 16) |
            (static_cast<uint32_t>(encoded[4]) << 24);

#if defined(_SERIALEMULATOR)
        if (mmsi < 227000000 || mmsi > 227999999)
            return {};
#endif

        int32_t rawLat = static_cast<int32_t>(encoded[5]) |
            (static_cast<int32_t>(encoded[6]) << 8) |
            (static_cast<int32_t>(encoded[7]) << 16) |
            (static_cast<int32_t>(encoded[8]) << 24);

        int32_t rawLon = static_cast<int32_t>(encoded[9]) |
            (static_cast<int32_t>(encoded[10]) << 8) |
            (static_cast<int32_t>(encoded[11]) << 16) |
            (static_cast<int32_t>(encoded[12]) << 24);

        double latitude = static_cast<double>(rawLat) / 1e7;
        double longitude = static_cast<double>(rawLon) / 1e7;

        std::lock_guard<std::mutex> lock(m_AISPendingMutex);
        m_AISPending.push_back({ mmsi, latitude, longitude, CTimeUtils::GetMs() });

        char buffer[128];
        snprintf(buffer, sizeof(buffer), "MMSI: %lu, Latitude: %.7f, Longitude: %.7f",
            static_cast<unsigned long>(mmsi), latitude, longitude);
        return std::string(buffer);
    }

    void ProcessAISUpdates()
    {
        std::vector<sAISUpdate> pending;
        {
            std::lock_guard<std::mutex> lock(m_AISPendingMutex);
            if (m_AISPending.empty())
                return;
            pending.swap(m_AISPending);
        }

        std::lock_guard<std::mutex> lock(m_AISContactsMutex);
        for (const auto& update : pending)
        {
            auto& contact = m_AISContacts[update.mmsi];
            if (contact.hasPosition)
            {
                sKinematics k = ComputeSogCog(
                    contact.latitude,
                    contact.longitude,
                    contact.lastSeenMs,
                    update.latitude,
                    update.longitude,
                    update.receivedMs);
                if (k.valid)
                {
                    contact.sogAcc += k.sogKnots;
                    contact.cogSinAcc += sin(k.cogDeg * 3.14159265358979323846 / 180.0);
                    contact.cogCosAcc += cos(k.cogDeg * 3.14159265358979323846 / 180.0);
                    contact.navSampleCount += 1;
                }
            }

            contact.mmsi = update.mmsi;
            contact.lastSeenMs = update.receivedMs;
            contact.dirty = true;
            contact.hasPosition = true;
            contact.latitudeAcc += update.latitude;
            contact.longitudeAcc += update.longitude;
            contact.sampleCount += 1;
        }
    }

    void PurgeAISContacts(uint64_t nowMs, uint64_t staleTimeoutMs)
    {
        std::lock_guard<std::mutex> lock(m_AISContactsMutex);
        for (auto it = m_AISContacts.begin(); it != m_AISContacts.end();)
        {
            const bool isStale = (nowMs > it->second.lastSeenMs) && ((nowMs - it->second.lastSeenMs) > staleTimeoutMs);
            if (isStale)
                it = m_AISContacts.erase(it);
            else
                ++it;
        }
    }

    std::vector<std::string> ConsumeAISMessages()
    {
        std::vector<std::string> messages;
        std::lock_guard<std::mutex> lock(m_AISContactsMutex);
        messages.reserve(m_AISContacts.size());

        for (auto& entry : m_AISContacts)
        {
            auto& contact = entry.second;
            if (!contact.dirty)
                continue;

            if (contact.sampleCount > 0)
            {
                contact.latitude = contact.latitudeAcc / static_cast<double>(contact.sampleCount);
                contact.longitude = contact.longitudeAcc / static_cast<double>(contact.sampleCount);
            }

            if (contact.navSampleCount > 0)
            {
                const double meanSog = contact.sogAcc / static_cast<double>(contact.navSampleCount);
                const double meanCogRad = atan2(contact.cogSinAcc, contact.cogCosAcc);
                double meanCog = meanCogRad * 180.0 / 3.14159265358979323846;
                if (meanCog < 0.0)
                    meanCog += 360.0;

                contact.sogKnots = meanSog;
                contact.cogDeg = meanCog;
                contact.hasKinematics = true;
            }

            const double sogToSend = contact.hasKinematics ? contact.sogKnots : std::numeric_limits<double>::quiet_NaN();
            const double cogToSend = contact.hasKinematics ? contact.cogDeg : std::numeric_limits<double>::quiet_NaN();

            messages.emplace_back(BuildAisAivdmType1(contact.mmsi, contact.latitude, contact.longitude, sogToSend, cogToSend));

            contact.latitudeAcc = 0.0;
            contact.longitudeAcc = 0.0;
            contact.sampleCount = 0;
            contact.sogAcc = 0.0;
            contact.cogSinAcc = 0.0;
            contact.cogCosAcc = 0.0;
            contact.navSampleCount = 0;
            contact.dirty = false;
        }

        return messages;
    }

    size_t GetAISContactCount() const
    {
        std::lock_guard<std::mutex> lock(m_AISContactsMutex);
        return m_AISContacts.size();
    }

private:
    struct sAISUpdate
    {
        uint32_t mmsi;
        double latitude;
        double longitude;
        uint64_t receivedMs;
    };

    struct sAISContact
    {
        uint32_t mmsi = 0;
        double latitude = 0.0;
        double longitude = 0.0;
        uint64_t lastSeenMs = 0;
        bool dirty = false;
        bool hasPosition = false;
        double latitudeAcc = 0.0;
        double longitudeAcc = 0.0;
        int sampleCount = 0;
        bool hasKinematics = false;
        double sogKnots = 0.0;
        double cogDeg = 0.0;
        double sogAcc = 0.0;
        double cogSinAcc = 0.0;
        double cogCosAcc = 0.0;
        int navSampleCount = 0;
    };

    mutable std::mutex m_AISPendingMutex;
    mutable std::mutex m_AISContactsMutex;
    std::vector<sAISUpdate> m_AISPending;
    std::map<uint32_t, sAISContact> m_AISContacts;
};

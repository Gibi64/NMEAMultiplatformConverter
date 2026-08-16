#include "CNMEATranslator.hpp"
#include "Stacking.h"
#include "CTimeUtils.hpp"
#include "Translate.hpp"
std::string CTranslate::TranlateRMC(CNMEATranslator::sCumulativeResult*s)
{
    double sPeed;
    double cOurse;
    if (s->m_Count <= 0)
        return "";

    double Lat = s->m_Data[0];
    double Lon = s->m_Data[1];
    double Dt = (s->LastTime - s->StartTime)/1000.0;
    if (Dt > 0)
    {
		sPeed = s->m_Data[2] / Dt * 1.94384; // m/s to knots
        cOurse = s->m_Data[3] / Dt; // le heading est renvoyé en degres par le PGN 127250, donc pas besoin de conversion
		long long lCourse = static_cast<long long>(cOurse)*10;
		lCourse = lCourse % 3600;
		cOurse = static_cast<double>(lCourse) / 10.0;
    }
    else
    {
        sPeed = 0;
		cOurse = 0;
    }
	auto utcTime = CTimeUtils::SystemDateTime(s->LastTime);
    char timeBuf[   16];
    int hundredths = utcTime.millisecond / 10;

    snprintf(timeBuf, sizeof(timeBuf),
        "%02d%02d%02d.%02d",
        utcTime.hour, utcTime.minute, utcTime.second, hundredths);

    int yy = utcTime.year % 100;
    char dateBuf[8];
    snprintf(dateBuf, sizeof(dateBuf),
        "%02d%02d%02d",
        utcTime.day, utcTime.month, yy);
    std::string latStr = CNMEATranslator::ToNMEA0183Coord(Lat, true);
    std::string lonStr = CNMEATranslator::ToNMEA0183Coord(Lon, false);
    char buf[128];
    snprintf(buf, sizeof(buf),
        "$GPRMC,%s,A,%s,%s,%.2f,%.2f,%s,,,A",
        timeBuf,
        latStr.c_str(),
        lonStr.c_str(),
       sPeed,
        cOurse,
        dateBuf);

    auto result = CNMEATranslator::CalculateNMEAChecksum(std::string(buf));

	// Reset the cumulative data after generating the RMC sentence
    s->m_Count = 1;
	s->m_Data[0] = Lat;
	s->m_Data[1] = Lon;
	s->m_Data[2] = 0; // Reset cumulative speed
	s->m_Data[3] = 0; // Reset cumulative course
	s->StartTime = s->LastTime;
    return result;
}
std::string CTranslate::TranslateMWV(CNMEATranslator::sCumulativeResult* s)
{
    double windSpeed;
    double windAngle;
    double Dt = (s->LastTime - s->StartTime)/1000.0;
    if (Dt > 0)
    {
        windSpeed = s->m_Data[0] / Dt * 1.94384; // m/s to knots
        windAngle = s->m_Data[1] / Dt * 180.0 / M_PI; // rad to deg si la CAN fornit des radians
		long long lAngle = static_cast<long long>(windAngle) * 10;
		lAngle = lAngle % 3600;
		windAngle = static_cast<double>(lAngle) / 10.0;
    }
    else
    {
        windSpeed = 0;
        windAngle = 0;
	}
    char buf[64];
    snprintf(buf, sizeof(buf), "$WIMWV,%.1f,R,%.1f,N,A", windAngle, windSpeed);
	// Reset the cumulative data after generating the MWV sentence
	s->m_Count = 1;
	s->m_Data[0] = 0; // Reset cumulative wind speed
	s->m_Data[1] = 0; // Reset cumulative wind angle
	s->StartTime = s->LastTime;

    return CNMEATranslator::CalculateNMEAChecksum(std::string(buf));
}
std::string CTranslate::TranslateDBT(CNMEATranslator::sCumulativeResult* s)
{

    double depthMeters;
    double Dt = (s->LastTime - s->StartTime)/1000.0;
    if (Dt > 0)
    {
		depthMeters = s->m_Data[0] / Dt; 
        }
    else
    {
        depthMeters = 0;
	}
    char buf[64];
    // Reset the cumulative data after generating the MWV sentence

    snprintf(buf, sizeof(buf),
        "$SDDBT,%.1f,f,%.1f,M,%.1f,F",
        depthMeters * 3.28084,
        depthMeters,
        depthMeters / 1.8288);	// Reset the cumulative data after generating the DBT sentence
    s->m_Count = 1;
	s->m_Data[0] = 0; // Reset cumulative Depth
    s->StartTime = s->LastTime;

    return CNMEATranslator::CalculateNMEAChecksum(std::string(buf));
}

std::string CTranslate::TranslateCGA(CNMEATranslator::sCumulativeResult* s)
{
	char timeBuf[16];
	double latitude = 0.0;
	double longitude = 0.0;

	CTimeUtils::sUTCTime t = CTimeUtils::SystemDateTime(s->LastTime);
	/////////////////////// Latitude and Longitude in NMEA 0183 format ///////////////////////
    snprintf(timeBuf, sizeof(timeBuf), "%02d%02d%02d", t.hour, t.minute, t.second);
    if (s->m_Count <= 0)
		return "";
	auto Dt = (s->LastTime - s->StartTime)/1000.0;
	latitude = s->m_Data[0]; // latitude in degrees
	longitude = s->m_Data[1]; // longitude in degrees
    std::string latStr = CNMEATranslator::ToNMEA0183Coord(s->m_Data[0], true);
    std::string lonStr = CNMEATranslator::ToNMEA0183Coord(s->m_Data[1], false);

    char buf[160];
    // Fix quality=1, satellites=08, HDOP=0.9, altitude/reference fixed for emulator use.
    snprintf(buf, sizeof(buf), "$GPGGA,%s,%s,%s,1,08,0.9,5.0,M,0.0,M,,",
        timeBuf,
        latStr.c_str(),
        lonStr.c_str());
	// Reset the cumulative data after generating the GGA sentence
	s->m_Count = 1;
	s->m_Data[0] = latitude;
	s->m_Data[1] = longitude;
	s->m_Data[2] = 0; // Reset cumulative speed
	s->m_Data[3] = 0; // Reset cumulative course
    return CNMEATranslator::CalculateNMEAChecksum(std::string(buf));
}
std::string CTranslate::TranslateHDT(CNMEATranslator::sCumulativeResult* s)
{
	//          Heading from true north, degrees 
    if (s->m_Count <= 0)
        return "";
    double sogSI = s->m_Data[0];
	double sog = 0.0;
	auto Dt = s->LastTime - s->StartTime;
    if (Dt > 0)
    {
		sog = sogSI; // deja en degres, pas besoin de conversion
    }
    else
    {
        sog = 0;
	}

    char buf[64];
    snprintf(buf, sizeof(buf), "$GPHDT,%.2f,T*00", sog);
	//// Reset the cumulative data after generating the HDT sentence

	s->m_Count = 1;
	s->m_Data[0] = sogSI; // Reset cumulative course
	s->StartTime = s->LastTime; // Reset last time to start time
    return CNMEATranslator::CalculateNMEAChecksum(std::string(buf));
}

std::string CTranslate::TranslateAisAivdmType1(CNMEATranslator::sCumulativeResult* R)

{
    std::vector<bool> bits;
    bits.reserve(168);
    auto mmsi = (long)R->m_Data[4];
	auto latitude = R->m_Data[0];
	auto longitude = R->m_Data[1];
	auto sogKnots = R->m_Data[2] * 1.94384;
	auto cogDeg = R->m_Data[3]*180/M_PI;

    const bool lonValid = (longitude >= -180.0 && longitude <= 180.0);
    const bool latValid = (latitude >= -90.0 && latitude <= 90.0);

    int32_t rawLon = lonValid ? static_cast<int32_t>(llround(longitude * 600000.0)) : 0x06791AC0;
    int32_t rawLat = latValid ? static_cast<int32_t>(llround(latitude * 600000.0)) : 0x03412140;
    uint16_t rawSog = 1023U;
    const double boundedSog = std::min<double>(sogKnots, 102.2);
    rawSog = static_cast<uint16_t>(llround(boundedSog * 10.0));
    

    uint16_t rawCog = 3600U;
    uint16_t rawHeading = 511U;
    rawCog = static_cast<uint16_t>(llround(cogDeg * 10.0));
    rawHeading = static_cast<uint16_t>(llround(cogDeg));
    if (rawHeading > 359U)
        rawHeading = 359U;
    CNMEATranslator::sAIS::AppendBits(bits, 1U, 6);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 2);
    CNMEATranslator::sAIS::AppendBits(bits, mmsi, 30);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 4);
    CNMEATranslator::sAIS::AppendBits(bits, 128U, 8);
    CNMEATranslator::sAIS::AppendBits(bits, rawSog, 10);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 1);
    CNMEATranslator::sAIS::AppendSignedBits(bits, rawLon, 28);
    CNMEATranslator::sAIS::AppendSignedBits(bits, rawLat, 27);
    CNMEATranslator::sAIS::AppendBits(bits, rawCog, 12);
    CNMEATranslator::sAIS::AppendBits(bits, rawHeading, 9);
    CNMEATranslator::sAIS::AppendBits(bits, 60U, 6);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 2);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 3);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 1);
    CNMEATranslator::sAIS::AppendBits(bits, 0U, 19);

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
        payload.push_back(CNMEATranslator::sAIS::ToAis6BitChar(value));
    }

    auto s= CNMEATranslator::CalculateNMEAChecksum("!AIVDM,1,1,,A," + payload + ",0");
    return s;
}

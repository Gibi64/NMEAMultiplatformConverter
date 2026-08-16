//////////////////////// Decodage des PGN et Cumul des valeurs
#include <vector>
#include "CGeographic.h"
#include "CNMEATranslator.hpp"
#include "Decode.hpp"
#include "Stacking.h"
#include "translate.hpp"
void  CDecode::DecodePGN129025(CNMEATranslator *pTranslator ,const std::vector<unsigned char>& encoded)
{
    if (encoded.size() < 8)
        return;

    int32_t rawLat = static_cast<int32_t>(encoded[0]) |
        (static_cast<int32_t>(encoded[1]) << 8) |
        (static_cast<int32_t>(encoded[2]) << 16) |
        (static_cast<int32_t>(encoded[3]) << 24);

    int32_t rawLon = static_cast<int32_t>(encoded[4]) |
        (static_cast<int32_t>(encoded[5]) << 8) |
        (static_cast<int32_t>(encoded[6]) << 16) |
        (static_cast<int32_t>(encoded[7]) << 24);

    CGeographic currentPoint(static_cast<double>(rawLat) / 1e7, static_cast<double>(rawLon) / 1e7);
    CStacking::StackNavData(pTranslator,&currentPoint);
}

void CDecode::DecodePGN126992(CNMEATranslator* pTranslator, const std::vector<unsigned char>& encoded)
{
	//////////// PGN 126992: Time & Date, encoded as 4 bytes for time in milliseconds since midnight, and 2 bytes for date in days since 1970-01-01.
    if (encoded.size() < 8)
        return;
    uint32_t timeMs =
        static_cast<uint32_t>(encoded[0]) |
        (static_cast<uint32_t>(encoded[1]) << 8) |
        (static_cast<uint32_t>(encoded[2]) << 16) |
        (static_cast<uint32_t>(encoded[3]) << 24);


    double seconds = timeMs / 1000.0;
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    int secs = static_cast<int>(seconds) % 60;
    int milliseconds = static_cast<int>(timeMs % 1000);
    auto UTCTime = CTimeUtils::SystemDateTime(CTimeUtils::GetMs());
    UTCTime.hour = hours;
    UTCTime.minute = minutes;
    UTCTime.second = secs;
    UTCTime.millisecond = milliseconds;
    CStacking::StackTime(pTranslator,&UTCTime);
}

void CDecode::DecodePGN128267(CNMEATranslator* pTranslator, const std::vector<unsigned char>& encoded)
{
	// PGN 128267: Depth, encoded as 4 bytes for depth in millimeters.
    if (encoded.size() < 8)
        return;

    // PGN 128267: depth is encoded in millimeters on bytes 1..4 (byte 0 is SID).
    uint32_t depthMm =
        static_cast<uint32_t>(encoded[1]) |
        (static_cast<uint32_t>(encoded[2]) << 8) |
        (static_cast<uint32_t>(encoded[3]) << 16) |
        (static_cast<uint32_t>(encoded[4]) << 24);

    double depthMeters = static_cast<double>(depthMm) / 1000.0;
    CStacking::StackDepthData(pTranslator, &depthMeters);
}

void CDecode::DecodePGN130306(CNMEATranslator* pTranslator, const std::vector<unsigned char>& encoded)
{
	/////////// PGN 130306: Wind Data, encoded as 2 bytes for angle in rad*10000, and 2 bytes for speed in m/s*100.
    if (encoded.size() < 6)
        return;

    uint16_t rawAngle =
        static_cast<uint16_t>(encoded[1]) |
        (static_cast<uint16_t>(encoded[2]) << 8);

    uint16_t rawSpeed =
        static_cast<uint16_t>(encoded[3]) |
        (static_cast<uint16_t>(encoded[4]) << 8);

    // PGN 130306: angle in rad*10000, speed in m/s*100.
    const double angleRad = static_cast<double>(rawAngle) / 10000.0;

    const double speedMS = static_cast<double>(rawSpeed) / 100.0;
	CNMEATranslator::sWindData windData = { speedMS, angleRad };
	CStacking::StackWindData(pTranslator, &windData);
    return;
}

void CDecode::DecodePGN128259(CNMEATranslator* pTranslator, const std::vector<unsigned char>& encoded)
{
	//////////// PGN 128259: Speed, encoded as 2 bytes for speed in m/s*100.
    uint16_t speed =
        static_cast<uint16_t>(encoded[1]) |
        (static_cast<uint16_t>(encoded[2]) << 8);
	double SpeedSI = static_cast<double>(speed) / 100.0 ; // Convert to m/s   
	CStacking::StackSpeedData(pTranslator, &SpeedSI);
    return;
}


void CDecode::DecodePGN127250(CNMEATranslator* pTranslator, const std::vector<unsigned char>& encoded)
{
	////////////////////////// PGN 127250: Vessel Heading, encoded as 2 bytes for heading in rad*10000.
    uint16_t heading =
        static_cast<uint16_t>(encoded[1]) |
        (static_cast<uint16_t>(encoded[2]) << 8);
	double dheading = static_cast<double>(heading) / 10000.0 ; // Convert rad*10000 to rad
    CStacking::StackHeadingData(pTranslator, &dheading);

    return;
}

void CDecode::DecodePGN129038(CNMEATranslator* pTranslator, const std::vector<unsigned char>& encoded)
{
	///////////////////////: PGN 129038: AIS Class B Position Report, encoded as 4 bytes for MMSI, 4 bytes for latitude in 1e-7 degrees, and 4 bytes for longitude in 1e-7 degrees.
    if (encoded.size() < 13)
        return;
    uint32_t mmsi = static_cast<uint32_t>(encoded[1]) |
        (static_cast<uint32_t>(encoded[2]) << 8) |
        (static_cast<uint32_t>(encoded[3]) << 16) |
        (static_cast<uint32_t>(encoded[4]) << 24);
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
    CGeographic currentPoint(latitude, longitude);
	//////////////////: Store the AIS contact information in the CNMEATranslator's otherBoats member.
	//////////////////// Do not stack more that 2 times for speed and heading, because AIS contacts are already stacked in the COtherBoats class.
	
	// On ne conserve que les 2 dernieres positions pour chaque MMSI, et on ne stacke pas plus de 2 fois pour la vitesse et le cap, car les contacts AIS sont d?j? empil?s dans la classe COtherBoats.
	auto& aisResult = pTranslator->GetAISResult(mmsi)->CumulativeResult;
	double course = 0.0;
	double speed = 0.0;
    if(aisResult.m_Count == 0)
    {
        aisResult.m_Count = 1;
        aisResult.StartTime = CTimeUtils::GetMs();
        aisResult.LastTime = aisResult.StartTime;
    }
    else
    {
        aisResult.m_Count++;
        aisResult.LastTime = CTimeUtils::GetMs();
		CGeographic oldPoint(aisResult.m_Data[0], aisResult.m_Data[1]);
		auto distance = oldPoint.DistanceTo(currentPoint);
		auto Dt = (aisResult.LastTime - aisResult.StartTime)/1000.0;
        aisResult.m_Count = 1;

	}
    aisResult.m_Data[0] = latitude;
	aisResult.m_Data[1] = longitude;
	aisResult.m_Data[2] = speed;
	aisResult.m_Data[3] = course;
    aisResult.m_Data[4] = mmsi;
	pTranslator->GetAISResult(mmsi)->LastSent = CTimeUtils::GetMs();
    std::string result = CTranslate::TranslateAisAivdmType1(&aisResult);
    pTranslator->GetUDP_Server()->send(result);
    write_log("Sending AIS : " + result + "\n");
	std::vector<std::map<long,CNMEATranslator::sAIS*>::iterator> toDelete;
    for (auto it = pTranslator->Get_Map_AIS()->begin(); it != pTranslator->Get_Map_AIS()->end(); it++)
    {

        if (CTimeUtils::GetMs() >= it->second->LastSent + 60000)
        {
            delete it->second;
            toDelete.push_back(it); 
        }
    }
    CTimeUtils::CPUSleep(2);
    aisResult.ReadyToSend = false;
    aisResult.m_Count = 1;
    for (auto& it : toDelete)
    {
        pTranslator->Get_Map_AIS()->erase(it);
    }
}
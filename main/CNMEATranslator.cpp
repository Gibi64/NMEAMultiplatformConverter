#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif
#include "CRouteurEmulateurNMEA.hpp"
#include "CNMEATranslator.hpp"
#include "InitLog.hpp"

#if defined(_ESP32)
#include "driver/twai.h"

bool CCanClient::Init()
    {
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, TWAI_MODE_NORMAL);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
            return false;

        return twai_start() == ESP_OK;
    }
bool CCanClient::Read(CCanFrame& frame)
    {
        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(10)) != ESP_OK)
            return false;

        frame.timestamp = CTimeUtils::GetMs();
        frame.pgn = (msg.identifier >> 8) & 0x3FFFF;
		frame.length = msg.data_length_code;
        frame.data.assign(msg.data, msg.data + msg.data_length_code);
        return true;
    }

#endif

CNMEATranslator::CNMEATranslator(CUDP_Broadcast_Server* udpServer)
{
    m_pUDPServer = udpServer;

    m_RMCMeanData.m_Count = 0;
    m_MapPGN[129025] = &CNMEATranslator::DecodePGN_129025;
    m_MapPGN[128267] = &CNMEATranslator::DecodePGN_128267;
    m_MapPGN[129038] = &CNMEATranslator::DecodePGN_129038;
    m_MapPGN[130306] = &CNMEATranslator::DecodePGN_130306;
    m_MapPGN[126992] = &CNMEATranslator::DecodePGN_126992;
    m_MapPGN[128259] = &CNMEATranslator::DecodePGN_128259;
    m_MapPGN[127250] = &CNMEATranslator::DecodePGN_127250;
    m_UTCTime = { 0, 0, 0, 0, 0, 0, 0 };
}

std::string CNMEATranslator::DecodePGN_129025(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    if (Encoded.size() < 8) return {};
    // Si le temps UTC n'est pas encore présent on ne traite pas
    if (pTranslator-> m_UTCTime.empty()) return "";
    // Reconstruction des entiers signés 32 bits (Little-Endian)
    int32_t rawLat = static_cast<int32_t>(Encoded[0]) |
        (static_cast<int32_t>(Encoded[1]) << 8) |
        (static_cast<int32_t>(Encoded[2]) << 16) |
        (static_cast<int32_t>(Encoded[3]) << 24);

    int32_t rawLon = static_cast<int32_t>(Encoded[4]) |
        (static_cast<int32_t>(Encoded[5]) << 8) |
        (static_cast<int32_t>(Encoded[6]) << 16) |
        (static_cast<int32_t>(Encoded[7]) << 24);

    double latitude = static_cast<double>(rawLat) / 1e7;
    double longitude = static_cast<double>(rawLon) / 1e7;

    // Retourne une chaîne formatée pour affichage ou log
    char buffer[128];
    sRMCData rmcData;
    rmcData.utcTime.year = pTranslator->m_UTCTime.year;
    rmcData.utcTime.month = pTranslator->m_UTCTime.month;
    rmcData.utcTime.day = pTranslator->m_UTCTime.day;
    rmcData.utcTime.hour = pTranslator->m_UTCTime.hour;
    rmcData.utcTime.minute = pTranslator->m_UTCTime.minute;
    rmcData.utcTime.second = pTranslator->m_UTCTime.second;
    rmcData.utcTime.millisecond = pTranslator->m_UTCTime.millisecond;
    rmcData.latitude = latitude;
    rmcData.longitude = longitude;

    pTranslator->StackMeanNavData(rmcData);

    //snprintf(buffer, sizeof(buffer), "$GPGLL,%s,%s,%2d%2d%2d.%2d,A", ToNMEA0183Coord(latitude, true).c_str(), ToNMEA0183Coord(longitude, false).c_str(), pUTC->hour, pUTC->minute, pUTC->second, pUTC->millisecond);
    //auto sReturn = CalculateNMEAChecksum(buffer);
    return "";
}
std::string CNMEATranslator::DecodePGN_129038(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    if (Encoded.size() < 13) return {};

    uint32_t mmsi = static_cast<uint32_t>(Encoded[1]) |
        (static_cast<uint32_t>(Encoded[2]) << 8) |
        (static_cast<uint32_t>(Encoded[3]) << 16) |
        (static_cast<uint32_t>(Encoded[4]) << 24);

    int32_t rawLat = static_cast<int32_t>(Encoded[5]) |
        (static_cast<int32_t>(Encoded[6]) << 8) |
        (static_cast<int32_t>(Encoded[7]) << 16) |
        (static_cast<int32_t>(Encoded[8]) << 24);

    int32_t rawLon = static_cast<int32_t>(Encoded[9]) |
        (static_cast<int32_t>(Encoded[10]) << 8) |
        (static_cast<int32_t>(Encoded[11]) << 16) |
        (static_cast<int32_t>(Encoded[12]) << 24);

    double latitude = static_cast<double>(rawLat) / 1e7;
    double longitude = static_cast<double>(rawLon) / 1e7;

    pTranslator->EnqueueAISUpdate(mmsi, latitude, longitude);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "MMSI: %ld, Latitude: %.7f, Longitude: %.7f", (long) mmsi, latitude, longitude);
    return std::string(buffer);

}
std::string CNMEATranslator::DecodePGN_130306(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    if (Encoded.size() < 6) return {};

    uint16_t rawAngle =
        static_cast<uint16_t>(Encoded[1]) |
        (static_cast<uint16_t>(Encoded[2]) << 8);

    uint16_t rawSpeed =
        static_cast<uint16_t>(Encoded[3]) |
        (static_cast<uint16_t>(Encoded[4]) << 8);


    pTranslator->StackMeanWindData(rawSpeed, rawAngle);
    return "";

}
std::string CNMEATranslator::DecodePGN_126992(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    if (Encoded.size() < 8) return {};

    uint32_t timeMs =
        static_cast<uint32_t>(Encoded[0]) |
        (static_cast<uint32_t>(Encoded[1]) << 8) |
        (static_cast<uint32_t>(Encoded[2]) << 16) |
        (static_cast<uint32_t>(Encoded[3]) << 24);

    uint32_t dateDays =
        static_cast<uint32_t>(Encoded[4]) |
        (static_cast<uint32_t>(Encoded[5]) << 8);

    double seconds = timeMs / 1000.0;
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    int secs = static_cast<int>(seconds) % 60;
    int milliseconds = static_cast<int>(timeMs % 1000);
    CTimeUtils::sDate s = CTimeUtils::SystemDate(dateDays);
    pTranslator->m_UTCTime.day = s.day;
    pTranslator->m_UTCTime.month = s.month;
    pTranslator->m_UTCTime.year = s.year;
    pTranslator->m_UTCTime.hour = hours;
    pTranslator->m_UTCTime.minute = minutes;
    pTranslator->m_UTCTime.second = secs;
    pTranslator->m_UTCTime.millisecond = milliseconds;
    return "";

}
std::string CNMEATranslator::DecodePGN_128259(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    // Vitesse par rapport à l'eau en m/s * 100
    if (Encoded.size() < 2) return {};

    /*uint16_t rawSpeed =
        (uint16_t)Encoded[0] |
        ((uint16_t)Encoded[1] << 8);*/

    return "";

}
std::string CNMEATranslator::DecodePGN_127250(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    // Heading in radians * 10000 deja defini dans le PGN 127250
    // fourni par l'orchestrateur de CAN2000 meme formulation
    if (Encoded.size() < 2) return {};

    /*uint16_t raw =
        (uint16_t)Encoded[0] |
        ((uint16_t)Encoded[1] << 8);

    double rad = raw / 10000.0;*/

    return "";

}
std::string CNMEATranslator::DecodePGN_128267(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator)
{
    // Depth in decimeters (0.1 m) * 10
    if (Encoded.size() < 8) return {};
    uint16_t rawDepth =
        (uint16_t)Encoded[0] |
        ((uint16_t)Encoded[1] << 8);
    pTranslator->StackMeanDepthData(rawDepth);
    return "";
}
std::string CNMEATranslator::CalculateNMEAChecksum(const std::string sentence)
{
    uint8_t checksum = 0;
    for (char c : sentence)
    {
        if (c != '$') checksum ^= c;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "*%02X\r\n", checksum);
    return sentence + buf;
}
std::string CNMEATranslator::ToNMEA0183Coord(double deg, bool isLat)
{
    char hemi = (isLat ? (deg >= 0 ? 'N' : 'S') : (deg >= 0 ? 'E' : 'W'));
    deg = fabs(deg);

    int d = (int)deg;
    double m = (deg - d) * 60.0;

    char buf[32];
    snprintf(buf, sizeof(buf), isLat ? "%02d%07.4f,%c" : "%03d%07.4f,%c", d, m, hemi);
    return std::string(buf);
}

void CNMEATranslator::StartLoop()
{
    m_bStarted = true;
}
int CNMEATranslator::GetDataCount(int pgn)
{
    switch (pgn)
    {
    case 129025: return 8; // Position, Rapid Update
    case 128267: return 8; // Water Depth
    case 129038: return 28; // AIS Class A Position Report
    case 130306: return 8; // Wind Data
    case 126992: return 8; // System Time
    case 128259: return 8; // Speed
    case 127250: return 8; // Heading
    default: return 0; // Unknown PGN
    }
}
void CNMEATranslator::LoopExternalReadData(void *Args)
{
#if defined(_SERIALEMULATOR)
    sArgumentsEmulator* pArgs = static_cast<sArgumentsEmulator*>(Args);
    CNMEATranslator* pTransltator = pArgs->pTranslator;
#else
#if defined(_WIN32) || defined(__linux__)
    sArgumentsSerial* pArgs = static_cast<sArgumentsSerial*>(Args);
    CNMEATranslator* pTransltator = pArgs->pTranslator;
    CSerialClient* pSerial = pArgs->pSerial;
#elif defined(_ESP32)
    sArgumentsCAN* pArgs = static_cast<sArgumentsCAN*>(Args);
    CNMEATranslator* pTransltator = pArgs->pTranslator;
    CCanClient* pCAN = pArgs->pCAN;        
#endif
#endif
    if (!pTransltator->m_bStarted) return;
    int CurrentPgn = 0;
    int HeaderSize = 4; // Taille de l'en-tête NMEA 2000
    int nbrOfBytes = 0;
    for (;;)
    {
#if defined(_SERIALEMULATOR)
		// Attention ici mutex sur la FIFO pour éviter les conflits avec le thread qui écrit dedans
        sArgumentsEmulator* pArgs = static_cast<sArgumentsEmulator*>(Args);
        CNMEATranslator* pTransltator = pArgs->pTranslator;
        CRouteurEmulateurNMEA* pRouteur = pArgs->pRouteur;
        {
            std::lock_guard<std::mutex> lock(pRouteur->GetMutex());

            if (!pRouteur->g_fifo_Send.empty())
            {
                auto& frame = pRouteur->g_fifo_Send.front();
                nbrOfBytes = frame.size();
                for (size_t i = 0; i < nbrOfBytes; ++i)
                    pTransltator->m_Received.push_back(frame[i]);
                pRouteur->g_fifo_Send.erase(pRouteur->g_fifo_Send.begin());
            }
        }
		auto bufferSize = pTransltator->m_Received.size();
#else
#if defined(_WIN32) || defined(__linux__)
        if (pSerial->GetSerialHandle() == INVALID_HANDLE_VALUE) break;
        nbrOfBytes = pSerial->getNbrOfBytes();

        ////////// on place la reception dans le buffer du traducteur
        if (nbrOfBytes)
        {
            for (size_t i = 0; i < nbrOfBytes; ++i)
            {
                BYTE c = static_cast<BYTE>(pSerial->getChar());
                pTransltator->m_Received.push_back(c);
            }
        }
        auto bufferSize = pTransltator->m_Received.size();
#elif defined(_ESP32)
        CCanFrame frame;
        if (pCAN->Read(frame))
        {
            nbrOfBytes = frame.length;

            for (int i = 0; i < nbrOfBytes; ++i)
                pTransltator->m_Received.push_back(frame.data[i]);
        }
        auto bufferSize = pTransltator->m_Received.size();
#endif
#endif

        // Draine le buffer: on decode toutes les trames completes disponibles avant d'attendre de nouvelles donnees.
        for (;;)
        {
            if (!CurrentPgn)
            {
                if (bufferSize < HeaderSize)
                    break;

                uint32_t header = (static_cast<uint32_t>(pTransltator->m_Received[0]) << 24) |
                    (static_cast<uint32_t>(pTransltator->m_Received[1]) << 16) |
                    (static_cast<uint32_t>(pTransltator->m_Received[2]) << 8) |
                    static_cast<uint32_t>(pTransltator->m_Received[3]);
                CurrentPgn = (header >> 8) & 0x3FFFF;
                pTransltator->m_Received.erase(pTransltator->m_Received.begin(), pTransltator->m_Received.begin() + HeaderSize);
                bufferSize = pTransltator->m_Received.size();
            }

            const int dataCount = GetDataCount(CurrentPgn);
            if (dataCount <= 0)
            {
                CurrentPgn = 0;
                continue;
            }

            if (bufferSize < static_cast<size_t>(dataCount))
                break;

            auto it = pTransltator->m_MapPGN.find(CurrentPgn);
            if (it != pTransltator->m_MapPGN.end())
            {
                std::string decodedMessage = it->second(std::vector<unsigned char>(pTransltator->m_Received.begin(), pTransltator->m_Received.begin() + dataCount), pTransltator);
            }

            pTransltator->m_Received.erase(pTransltator->m_Received.begin(), pTransltator->m_Received.begin() + dataCount);
            CurrentPgn = 0; // Reset for next message
            bufferSize = pTransltator->m_Received.size();
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
CNMEATranslator::sUTCTime CNMEATranslator::DecodeZDA(std::string zda)
{
    int hour=0, minute=0, second=0, msec=0, day=0, month=0, year=0;
    auto iComma = zda.find(',');
    zda = zda.substr(iComma + 1);
    int nbCommma = -1;
    size_t FirstPos = 0;
    for (;;)
    {
        iComma = zda.find(',', FirstPos);
        if (iComma == std::string::npos) iComma = zda.length();
        nbCommma++;
        std::string token = zda.substr(FirstPos, iComma - FirstPos);
        FirstPos = iComma + 1;
        switch (nbCommma)
        {
        case 0: // Heure
            hour = atoi(token.substr(0, 2).c_str());
            minute = atoi(token.substr(2, 2).c_str());
            second = atoi(token.substr(4, 2).c_str());
            if (token.length() > 6 && token[6] == '.')
            {
                // Convertir en millisecondes
                double frac = atof(token.substr(6).c_str());
                msec = (int)(frac * 1000.0);
            }
            else
            {
                msec = 0;
            }
            break;
        case 1: // date
            day = atoi(token.substr(0, 2).c_str());
            month = atoi(token.substr(2, 2).c_str());
            year = atoi(token.substr(4, 4).c_str());
            break;
        default:
            break;
        }
        if (FirstPos >= zda.length()) break;
    }
    return CNMEATranslator::sUTCTime{ year, month, day, hour, minute, second, msec };
}
uint64_t CNMEATranslator::ToEpoch2001ms(const sUTCTime& t)
{
    // Nombre de jours par mois (non bissextile)
    static const int mdays[12] =
    { 31,28,31,30,31,30,31,31,30,31,30,31 };

    // Calcul des jours depuis 2001-01-01
    int y = t.year;
    int m = t.month;
    int d = t.day;

    // Jours des années complètes
    uint64_t days = 0;
    for (int year = 2001; year < y; ++year)
    {
        days += ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            ? 366 : 365;
    }

    // Jours des mois complets de l'année courante
    for (int month = 1; month < m; ++month)
    {
        days += mdays[month - 1];
        if (month == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)))
            days += 1; // février bissextile
    }

    // Jours du mois courant
    days += (d - 1);

    // Conversion en millisecondes
    uint64_t ms = days * 24ULL * 3600ULL * 1000ULL;
    ms += t.hour * 3600ULL * 1000ULL;
    ms += t.minute * 60ULL * 1000ULL;
    ms += t.second * 1000ULL;
    ms += t.millisecond;

    return ms;
}

    CNMEATranslator::sNavDelta CNMEATranslator::ComputeDelta(const sRMCData& p1, const sRMCData& p2)
{
    // Conversion degrés → radians
    auto deg2rad = [](double d) { return d * M_PI / 180.0; };

    double lat1 = deg2rad(p1.latitude);
    double lon1 = deg2rad(p1.longitude);
    double lat2 = deg2rad(p2.latitude);
    double lon2 = deg2rad(p2.longitude);

    // Différences
    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    // Latitude moyenne (géométriquement justifié pour petits déplacements)
    double latMean = (lat1 + lat2) / 2.0;

    // Rayon terrestre
    constexpr double R = 6371000.0; // mètres

    // Projection locale (plan tangent)
    double dNorth = R * dLat;
    double dEast = R * cos(latMean) * dLon;

    // Distance horizontale
    double distance = sqrt(dNorth * dNorth + dEast * dEast);

    // Delta temps en secondes
    uint64_t t1 = ToEpoch2001ms(p1.utcTime);
    uint64_t t2 = ToEpoch2001ms(p2.utcTime);

    double dt = (double)(t2 - t1) / 1000.0; // en secondes

    if (dt <= 0.0) dt = 1.0; // sécurité
    // Vitesse en m/s puis en nœuds
    double speedMS = distance / dt;
    double speedKnots = speedMS / 0.514444;

    // Cap (0 = Nord, 90 = Est)
    double courseRad = atan2(dEast, dNorth);
    double courseDeg = courseRad * 180.0 / M_PI;
    if (courseDeg < 0) courseDeg += 360.0;

    return { distance, speedKnots, courseDeg };
}

bool CNMEATranslator::StackMeanNavData(sRMCData data)
{
    // On memrise le nombre de points pour pondérer les moyennes
    // On ecrase tous les points sauf le dernier apres ponderation

    if (!m_RMCMeanData.m_Count)
    {
        data.speedKnots = 0;
        data.courseOverGround = 0;
        m_RMCMeanData.RMCMeanData = data;
        m_RMCMeanData.m_Count = 1;
        return false;
    }
    auto& p1 = m_RMCMeanData.RMCMeanData;
    sNavDelta delta = ComputeDelta(p1, data);
    double MeanSpeed = (m_RMCMeanData.m_Count * p1.speedKnots + delta.speedKnots)
        / (m_RMCMeanData.m_Count + 1);
    double MeanCourse = (m_RMCMeanData.m_Count * p1.courseOverGround + delta.courseDegrees)
        / (m_RMCMeanData.m_Count + 1);
    m_RMCMeanData.m_Count++;
    p1.latitude = data.latitude;
    p1.longitude = data.longitude;
    p1.speedKnots = MeanSpeed;
    p1.courseOverGround = MeanCourse;
    p1.utcTime = data.utcTime;
    //std::cout << "Distance: " << delta.distanceMeters << " m, Speed: " << delta.speedKnots << " kn, Course: " << delta.courseDegrees << " deg" << std::endl;
return true;
}
bool CNMEATranslator::StackMeanDepthData(double depth)
{
    if (!m_RMCMeanData.m_Count)
    {
        m_RMCMeanData.RMCMeanData.speedKnots = 0;
        m_RMCMeanData.RMCMeanData.courseOverGround = 0;
        m_RMCMeanData.m_Count = 1;
        return false;
    }
    m_RMCMeanData.RMCMeanData.speedKnots = (m_RMCMeanData.m_Count * m_RMCMeanData.RMCMeanData.speedKnots + depth)
        / (m_RMCMeanData.m_Count + 1);
    m_RMCMeanData.m_Count++;
    return true;
}
bool CNMEATranslator::StackMeanWindData(double speed, double angle)
{
    if (!m_RMCMeanData.m_Count)
    {
        m_RMCMeanData.RMCMeanData.speedKnots = 0;
        m_RMCMeanData.RMCMeanData.courseOverGround = 0;
        m_RMCMeanData.m_Count = 1;
        return false;
    }
    m_RMCMeanData.RMCMeanData.speedKnots = (m_RMCMeanData.m_Count * m_RMCMeanData.RMCMeanData.speedKnots + speed)
        / (m_RMCMeanData.m_Count + 1);
    m_RMCMeanData.RMCMeanData.courseOverGround = (m_RMCMeanData.m_Count * m_RMCMeanData.RMCMeanData.courseOverGround + angle)
        / (m_RMCMeanData.m_Count + 1);
    m_RMCMeanData.m_Count++;
    return true;
}
std::string CNMEATranslator::BuildRMC()
{
    const auto& t = m_RMCMeanData.RMCMeanData.utcTime;

    // Heure hhmmss.sss
    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf),
        "%02d%02d%02d.%03d",
        t.hour, t.minute, t.second, t.millisecond);

    // Date ddmmyy (NMEA = année sur 2 chiffres)
    int yy = t.year % 100;
    char dateBuf[8];
    snprintf(dateBuf, sizeof(dateBuf),
        "%02d%02d%02d",
        t.day, t.month, yy);

    // Coordonnées NMEA0183
    std::string latStr = ToNMEA0183Coord(m_RMCMeanData.RMCMeanData.latitude, true);  // "ddmm.mmmm,N/S"
    std::string lonStr = ToNMEA0183Coord(m_RMCMeanData.RMCMeanData.longitude, false); // "dddmm.mmmm,E/W"

    // On sépare valeur et hémisphère
    auto splitCoord = [](const std::string& s)
        {
            auto commaPos = s.find(',');
            return std::make_pair(s.substr(0, commaPos),
                s.substr(commaPos + 1)); // après la virgule
        };

    auto lat = splitCoord(latStr);
    auto lon = splitCoord(lonStr);

    // Phrase RMC sans checksum
    char buf[128];
    snprintf(buf, sizeof(buf),
        "$GPRMC,%s,A,%s,%s,%s,%s,%.2f,%.2f,%s,,,A",
        timeBuf,
        lat.first.c_str(), lat.second.c_str(),
        lon.first.c_str(), lon.second.c_str(),
        m_RMCMeanData.RMCMeanData.speedKnots,
        m_RMCMeanData.RMCMeanData.courseOverGround,
        dateBuf);

    // Ajout du checksum
    auto result = CalculateNMEAChecksum(std::string(buf));
    m_RMCMeanData.m_Count = 0; // Reset count after building RMC
    //std::cout << "Built RMC: " << result << std::endl;
    return result;
}
std::string CNMEATranslator::BuildDBT()
{
    double depthMeters = m_RMCMeanData.RMCMeanData.speedKnots; // Using speedKnots to store mean depth for this example
    char buf[64];
    snprintf(buf, sizeof(buf), "$SDDBT,%.1f,f,%.1f,M,%.1f,F", depthMeters * 3.28084, depthMeters, depthMeters * 3.28084);
    auto result = CalculateNMEAChecksum(std::string(buf));
    //std::cout << "Built DBT: " << result << std::endl;
    return result;
}
    std::string CNMEATranslator::BuildMWV()
    {
        double windSpeed = m_RMCMeanData.RMCMeanData.speedKnots; // Using speedKnots to store mean wind speed for this example
        double windAngle = m_RMCMeanData.RMCMeanData.courseOverGround; // Using courseOverGround to store mean wind angle for this example
        char buf[64];
        snprintf(buf, sizeof(buf), "$WIMWV,%.1f,R,%.1f,N,A", windAngle, windSpeed);
        auto result = CNMEATranslator::CalculateNMEAChecksum(std::string(buf));
        //std::cout << "Built MWV: " << result << std::endl;
        return result;
    }


void CNMEATranslator::LoopTCP_UDPSend(void* Args)
{
    auto pArgs = static_cast<sArgumentsUDP*>(Args);
    CNMEATranslator* pTranslator = pArgs->pTranslator;
    if (!pTranslator->m_bStarted) return;
    unsigned long long  startTime = CTimeUtils::GetMs();
    for (;;)
    {
        if (CTimeUtils::GetMs() - startTime > pArgs->Timer_ms)
        {
            std::string rmc = pTranslator->BuildRMC();
            if (!rmc.empty())
            {
                write_log("Sending RMC: " + rmc + "\n");
                if (pTranslator->m_pUDPServer)
                {
                    pTranslator->m_pUDPServer->send(rmc);
                }
            }
            std::string dbt = pTranslator->BuildDBT();
            if (!dbt.empty())
            {
                //std::cout << "Sending DBT: " << dbt << std::endl;
                write_log("Sending DBT: " + dbt + "\n");
                if (pTranslator->m_pUDPServer)
                {
                    pTranslator->m_pUDPServer->send(dbt);
                }
            }
            std::string mwv = pTranslator->BuildMWV();
            if (!mwv.empty())
            {
                //std::cout << "Sending MWV: " << mwv << std::endl;
                write_log("Sending MWV: " + mwv + "\n");
                if (pTranslator->m_pUDPServer)
                {
                    pTranslator->m_pUDPServer->send(mwv);
                }
            }
            startTime = CTimeUtils::GetMs();
        }
        CTimeUtils::CPUSleep(2);
    }
}

void CNMEATranslator::EnqueueAISUpdate(uint32_t mmsi, double latitude, double longitude)
{
    std::lock_guard<std::mutex> lock(m_AISPendingMutex);
    m_AISPending.push_back({ mmsi, latitude, longitude, CTimeUtils::GetMs() });
}

void CNMEATranslator::ProcessAISUpdates()
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
        contact.mmsi = update.mmsi;
        contact.latitude = update.latitude;
        contact.longitude = update.longitude;
        contact.lastSeenMs = update.receivedMs;
        contact.dirty = true;
    }
}

void CNMEATranslator::PurgeAISContacts(uint64_t nowMs, uint64_t staleTimeoutMs)
{
    std::lock_guard<std::mutex> lock(m_AISContactsMutex);
    for (auto it = m_AISContacts.begin(); it != m_AISContacts.end(); )
    {
        const bool isStale = (nowMs > it->second.lastSeenMs) && ((nowMs - it->second.lastSeenMs) > staleTimeoutMs);
        if (isStale)
            it = m_AISContacts.erase(it);
        else
            ++it;
    }
}

size_t CNMEATranslator::GetAISContactCount()
{
    std::lock_guard<std::mutex> lock(m_AISContactsMutex);
    return m_AISContacts.size();
}

void CNMEATranslator::LoopAIS(void* Args)
{
    auto pArgs = static_cast<sArgumentsAIS*>(Args);
    CNMEATranslator* pTranslator = pArgs->pTranslator;
    if (!pTranslator->m_bStarted) return;

    uint64_t lastLogTimeMs = CTimeUtils::GetMs();
    for (;;)
    {
        pTranslator->ProcessAISUpdates();
        pTranslator->PurgeAISContacts(CTimeUtils::GetMs(), static_cast<uint64_t>(pArgs->StaleTimeout_ms));

        const uint64_t nowMs = CTimeUtils::GetMs();
        if (nowMs - lastLogTimeMs >= 1000)
        {
            write_log("AIS contacts active: " + std::to_string(pTranslator->GetAISContactCount()) + "\n");
            lastLogTimeMs = nowMs;
        }

        CTimeUtils::CPUSleep(pArgs->Timer_ms > 0 ? pArgs->Timer_ms : 20);
    }
}

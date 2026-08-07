#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif
#include "CCANPGN.hpp"
#include <math.h>
#include "CLaunchThread.hpp"
#include "CRouteurEmulateurNMEA.hpp"
#include <math.h>
#include "CTimeUtils.hpp"  
CNMEACAN::~CNMEACAN()
    {
        if (m_pLauchThread)
        {
            delete m_pLauchThread;
            m_pLauchThread = nullptr;
        }
    }

double CNMEACAN::BoundedRand(double range_min, double range_max, int digit)
{
    // 1. Génération d'un double aléatoire uniforme entre range_min et range_max
    double raw = range_min + (static_cast<double>(rand()) / RAND_MAX) * (range_max - range_min);

    // 2. Arrondi à 'digit' décimales (ex: digit = 3 -> facteur 1000)
    double factor = pow(10.0, digit);
    return round(raw * factor) / factor;
}
std::string CNMEACAN::ToNMEA0183Coord(double deg, bool isLat)
{
    char hemi = (isLat ? (deg >= 0 ? 'N' : 'S') : (deg >= 0 ? 'E' : 'W'));
    deg = fabs(deg);

    int d = (int)deg;
    double m = (deg - d) * 60.0;

    char buf[32];
    snprintf(buf, sizeof(buf), isLat ? "%02d%07.4f,%c" : "%03d%07.4f,%c", d, m, hemi);
    return std::string(buf);
}
void CNMEACAN::StartLoop()
{
    m_bStarted = true;
	m_pLauchThread = new CLaunchThread(&CNMEACAN::LoopProducer, this);
    //std::thread loopThread(&CNMEACAN::LoopProducer, this);
}
uint32_t CNMEACAN::getHeader()
{
    return m_Header;
}
void CNMEACAN::setHeader()
{
    m_Header = ((static_cast<uint32_t>(m_Priority) & 0x07) << 26) |
        ((m_PGN & 0x3FFFF) << 8) |
        (static_cast<uint32_t>(m_SourceAddress) & 0xFF);
}

void CNMEACAN::LoopProducer(void *pArg)
{
    CNMEACAN* pCAN = reinterpret_cast<CNMEACAN *>( pArg);

    if (!pCAN->m_pRouteur || !pCAN->m_bStarted)
        return;
    auto lastTime = CTimeUtils::GetMs();
    while (!pCAN->m_pRouteur->g_bStopThread.load())
    {
        auto now = CTimeUtils::GetMs();
        double dt = (now - lastTime);

        if (dt >= 100)
        {
            lastTime = now;
            pCAN->GenerateRandomData(dt / 1000, 1);
            pCAN->send(); // pousse dans la FIFO
        }

        CTimeUtils::CPUSleep(2);
    }
}
void CNMEACAN::send()
{
    // On s'assure que le routeur est valide avant de tenter d'envoyer
    if (!m_pRouteur)
        return;
    // 1. On s'assure que les données binaire sont à jour dans m_Encode
    encode();

    // 2. Préparation d'un buffer unique pour contenir TOUT le message (12 octets)
    std::vector<unsigned char> fullFrame;

    // 3. Injection du Header (4 octets / 32 bits) au début de la trame
    fullFrame.push_back(static_cast<unsigned char>((m_Header >> 24) & 0xFF));
    fullFrame.push_back(static_cast<unsigned char>((m_Header >> 16) & 0xFF));
    fullFrame.push_back(static_cast<unsigned char>((m_Header >> 8) & 0xFF));
    fullFrame.push_back(static_cast<unsigned char>(m_Header & 0xFF));

    // 4. Ajout immédiat des 8 octets du Payload juste à la suite
    fullFrame.insert(fullFrame.end(), m_Encode.begin(), m_Encode.end());

    // 5. Envoi des 12 octets en un seul bloc indivisible dans le FIFO du routeur
    m_pRouteur->PushFIFO(fullFrame.data(), fullFrame.size());

}

///////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_128259::CPGN_CNMEA_128259(CRouteurEmulateurNMEA* pRouteur)
{
    m_Priority = 3;
    m_SourceAddress = 0x06;
    m_PGN = 128259;
    m_pRouteur = pRouteur;

    setHeader();
    m_Encode.resize(8, 0xFF);

    m_SpeedKnots = 0.0;
    StartLoop();
}
void CPGN_CNMEA_128259::GenerateRandomData(double dt, double tf)
{
}
void CPGN_CNMEA_128259::encode()
{
    m_Encode.resize(8);

    // Speed in m/s * 100
    double speedMS = m_SpeedKnots * 0.514444;
    uint16_t rawSpeed = (uint16_t)(speedMS * 100.0);

    m_Encode[0] = rawSpeed & 0xFF;
    m_Encode[1] = rawSpeed >> 8;

    // Water referenced
    m_Encode[2] = 0x00;

    // Reserved
    m_Encode[3] = 0xFF;
    m_Encode[4] = 0xFF;
    m_Encode[5] = 0xFF;
    m_Encode[6] = 0xFF;
    m_Encode[7] = 0xFF;
}
void CPGN_CNMEA_128259::send()
{
    encode();
    CNMEACAN::send();
}

void CPGN_CNMEA_128259::setSpeed(double knots)
{
    m_SpeedKnots = knots;
}


//////////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_127250::CPGN_CNMEA_127250(CRouteurEmulateurNMEA* pRouteur)
{
    m_Priority = 3;
    m_SourceAddress = 0x07;
    m_PGN = 127250;
    m_pRouteur = pRouteur;

    setHeader();
    m_Encode.resize(8, 0xFF);

    m_HeadingDeg = 0.0;
    StartLoop();
}

void CPGN_CNMEA_127250::GenerateRandomData(double dt, double tf)
{
}

void CPGN_CNMEA_127250::encode()
{
    m_Encode.resize(8);

    // heading en radians * 10000
    double rad = m_HeadingDeg * 0.017453292519943295; // PI/180
    uint16_t raw = (uint16_t)(rad * 10000.0);

    m_Encode[0] = raw & 0xFF;
    m_Encode[1] = raw >> 8;

    // deviation + variation non utilisées
    m_Encode[2] = 0xFF;
    m_Encode[3] = 0xFF;
    m_Encode[4] = 0xFF;
    m_Encode[5] = 0xFF;
    m_Encode[6] = 0xFF;
    m_Encode[7] = 0xFF;
}
void CPGN_CNMEA_127250::send()
{
    encode();
    CNMEACAN::send();
}
void CPGN_CNMEA_127250::setHeading(double heading)
{
    m_HeadingDeg = heading;
}//////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_126992::CPGN_CNMEA_126992(CRouteurEmulateurNMEA* pRouteur)
{
    m_Priority = 3;
    m_SourceAddress = 0x05;
    m_PGN = 126992;
    m_pRouteur = pRouteur;
    setHeader();
    m_Encode.resize(8, 0xFF);
    StartLoop();
	m_Time = 0;
	m_Date = 0;
   
}
void CPGN_CNMEA_126992::encode()
{
    m_Encode.resize(8);

    // Time (ms)
    m_Encode[0] = m_Time & 0xFF;
    m_Encode[1] = (m_Time >> 8) & 0xFF;
    m_Encode[2] = (m_Time >> 16) & 0xFF;
    m_Encode[3] = (m_Time >> 24) & 0xFF;

    // Date (days)
    m_Encode[4] = m_Date & 0xFF;
    m_Encode[5] = (m_Date >> 8) & 0xFF;

    // Local offset + reserved
    m_Encode[6] = 0x00;
    m_Encode[7] = 0xFF;
}
void CPGN_CNMEA_126992::GenerateRandomData(double dt, double tf)
{
#if defined(_ESP32)
    time_t now;
    time(&now);
    struct tm t;
    gmtime_r(&now, &t);

    // Millisecondes depuis minuit
    m_Time =
        (t.tm_hour * 3600 +
            t.tm_min * 60 +
            t.tm_sec) * 1000;

    // Jours depuis epoch (1970-01-01)
    m_Date = static_cast<uint32_t>(now / 86400);
#else
    SYSTEMTIME st;
    GetSystemTime(&st);   // UTC, parfait pour NMEA

    // Millisecondes depuis minuit
    m_Time =
        (st.wHour * 3600 +
            st.wMinute * 60 +
            st.wSecond) * 1000 +
        st.wMilliseconds;

    // Jours depuis 1970
    // On convertit SYSTEMTIME → jours depuis 1970 avec ta propre classe
    tm t = {};
    t.tm_year = st.wYear - 1900;
    t.tm_mon = st.wMonth - 1;
    t.tm_mday = st.wDay;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;

    // _mkgmtime = conversion UTC → time_t (epoch 1970)
    time_t seconds = _mkgmtime(&t);

    m_Date = static_cast<uint32_t>(seconds / 86400);
#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_129025::CPGN_CNMEA_129025(CRouteurEmulateurNMEA* pRouteur)
{
    m_Priority = 3; // Example priority
    m_SourceAddress = 0x01; // Example source address
    m_PGN = 129025;
    m_pRouteur = pRouteur;
    setHeader();
    // 1. Latitude initiale : entre 46.000 et 48.000
    m_Latitude = BoundedRand(46, 48, 3);

    // 2. Longitude initiale : entre 0.000 et 10.000
    m_Longitude = BoundedRand(0, 10, 3);

    // 3. Vitesse : entre 20 et 50 / 10 => 2.0 à 5.0 nœuds
    m_SpeedKnots = BoundedRand(2, 5, 1);

    // 4. Cap : entre 0 et 360 degrés
    m_HeadingDeg = BoundedRand(0, 360, 0);
    m_pSpeed = new CPGN_CNMEA_128259(pRouteur);
    m_pHeading = new CPGN_CNMEA_127250(pRouteur);
    m_pTime = new CPGN_CNMEA_126992(pRouteur);

    StartLoop();

}
CPGN_CNMEA_129025::~CPGN_CNMEA_129025()
{
    delete m_pSpeed;
    delete m_pHeading;
    delete m_pTime;
}
void CPGN_CNMEA_129025::GenerateRandomData(double trueDeltaTime, double timeFactor) 
{
    m_HeadingTimerAcc += trueDeltaTime;
    m_SpeedTimerAcc += trueDeltaTime;

    double simulatedDeltaT = trueDeltaTime * timeFactor;

    // --- 1. Variation lente de la vitesse ---
    if (m_SpeedTimerAcc >= 3.0)   // toutes les 3 secondes
    {
        m_SpeedTimerAcc = 0.0;

        // Variation douce
        double speedDelta = ((rand() % 61) - 30) / 100.0;  // ±0.30 kn
        m_SpeedKnots += speedDelta;

        if (m_SpeedKnots < 0.1) m_SpeedKnots = 0.1;
        if (m_SpeedKnots > 8.0) m_SpeedKnots = 8.0;
    }

    // --- 2. Variation lente du cap ---
    if (m_HeadingTimerAcc >= 45.0)   // toutes les 45 secondes
    {
        m_HeadingTimerAcc = 0.0;

        // Variation franche
        double headingDelta = ((rand() % 121) - 60);  // ±60°
        m_HeadingDeg += headingDelta;

        // Normalisation
        if (m_HeadingDeg < 0.0)   m_HeadingDeg += 360.0;
        if (m_HeadingDeg >= 360.0) m_HeadingDeg -= 360.0;
    }

    // --- 3. Mise à jour de la position ---
    double speedDegPerSec = (m_SpeedKnots / 60.0) / 3600.0;
    double headingRad = m_HeadingDeg * 3.14159265358979323846 / 180.0;

    m_Latitude += speedDegPerSec * cos(headingRad) * simulatedDeltaT;
    m_Longitude += speedDegPerSec * sin(headingRad) * simulatedDeltaT;

    // --- 4. Mise à jour des PGN esclaves ---
    m_pSpeed->setSpeed(m_SpeedKnots);
    m_pHeading->setHeading(m_HeadingDeg);
}

void CPGN_CNMEA_129025::encode()
{
    // Conversion des degrés en multiples de 10^-7 degrés (Format NMEA 2000)
    int32_t rawLat = static_cast<int32_t>(m_Latitude * 1e7);
    int32_t rawLon = static_cast<int32_t>(m_Longitude * 1e7);

    // Resize du vector à 8 octets au cas où
    m_Encode.resize(8);

    // Encodage en Little-Endian (Intel)
    // Latitude (Bytes 0 à 3)
    m_Encode[0] = static_cast<unsigned char>(rawLat & 0xFF);
    m_Encode[1] = static_cast<unsigned char>((rawLat >> 8) & 0xFF);
    m_Encode[2] = static_cast<unsigned char>((rawLat >> 16) & 0xFF);
    m_Encode[3] = static_cast<unsigned char>((rawLat >> 24) & 0xFF);

    // Longitude (Bytes 4 à 7)
    m_Encode[4] = static_cast<unsigned char>(rawLon & 0xFF);
    m_Encode[5] = static_cast<unsigned char>((rawLon >> 8) & 0xFF);
    m_Encode[6] = static_cast<unsigned char>((rawLon >> 16) & 0xFF);
    m_Encode[7] = static_cast<unsigned char>((rawLon >> 24) & 0xFF);
}

// Getters pour récupérer les valeurs après décodage
double CPGN_CNMEA_129025::getLatitude() 
{ 
    return m_Latitude; 
}
double CPGN_CNMEA_129025::getLongitude() 
{ 
    return m_Longitude; 
}
////////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_128267::CPGN_CNMEA_128267(CRouteurEmulateurNMEA* pRouteur)
{
    m_Priority = 3;
    m_SourceAddress = 0x02; // Exemple adresse Sondeur
    m_PGN = 128267;         // 0x1F00D
    setHeader();
    m_pRouteur = pRouteur;

    m_DepthMeters = 0.0;
    m_Encode.resize(8, 0xFF);
    StartLoop();
}
void CPGN_CNMEA_128267::setDepth(double depthMeters)
{
    m_DepthMeters = depthMeters;
}

void CPGN_CNMEA_128267::encode()
{
    m_Encode.resize(8, 0xFF);

    // Convertir la profondeur de mètres en millimètres
    uint32_t depthMm = static_cast<uint32_t>(m_DepthMeters * 1000.0);

    m_Encode[0] = 0x01; // SID (Sequence ID)

    // Profondeur sur 4 octets (Bytes 1 à 4) - Little Endian
    m_Encode[1] = static_cast<unsigned char>(depthMm & 0xFF);
    m_Encode[2] = static_cast<unsigned char>((depthMm >> 8) & 0xFF);
    m_Encode[3] = static_cast<unsigned char>((depthMm >> 16) & 0xFF);
    m_Encode[4] = static_cast<unsigned char>((depthMm >> 24) & 0xFF);

    // Offset (tirant d'eau) & Max Range non spécifiés (0x00 / 0xFF)
    m_Encode[5] = 0x00;
    m_Encode[6] = 0x00;
    m_Encode[7] = 0xFF;
}


double CPGN_CNMEA_128267::getDepth() 
{ 
    
    return m_DepthMeters; 
}
void CPGN_CNMEA_128267::GenerateRandomData(double trueDeltaTime, double timeFactor)
{
    m_DepthMeters = BoundedRand(0, 50, 1);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_129038::CPGN_CNMEA_129038(CRouteurEmulateurNMEA* pRouteur, CPGN_CNMEA_129025* pOwnShip)
{
    m_Priority = 4;
    m_SourceAddress = 0x04; // Transpondeur AIS
    m_PGN = 129038;         // 0x1F016
    setHeader();
    m_pRouteur = pRouteur;
    m_pOwnShip = pOwnShip;

    m_MMSI = 0;
    m_Latitude = 0.0;
    m_Longitude = 0.0;
    m_Encode.resize(28, 0xFF); // Payload complet AIS
    StartLoop();

}

void CPGN_CNMEA_129038::setTargetData(uint32_t mmsi, double latitude, double longitude) 
{
    m_MMSI = mmsi;
    m_Latitude = latitude;
    m_Longitude = longitude;
}

void CPGN_CNMEA_129038::encode()
{
    m_Encode.resize(28, 0xFF);

    int32_t rawLat = static_cast<int32_t>(m_Latitude * 1e7);
    int32_t rawLon = static_cast<int32_t>(m_Longitude * 1e7);

    m_Encode[0] = 0x01; // Message ID / Sequence ID

    // MMSI (Bytes 1 à 4)
    m_Encode[1] = static_cast<unsigned char>(m_MMSI & 0xFF);
    m_Encode[2] = static_cast<unsigned char>((m_MMSI >> 8) & 0xFF);
    m_Encode[3] = static_cast<unsigned char>((m_MMSI >> 16) & 0xFF);
    m_Encode[4] = static_cast<unsigned char>((m_MMSI >> 24) & 0xFF);

    // Latitude (Bytes 5 à 8)
    m_Encode[5] = static_cast<unsigned char>(rawLat & 0xFF);
    m_Encode[6] = static_cast<unsigned char>((rawLat >> 8) & 0xFF);
    m_Encode[7] = static_cast<unsigned char>((rawLat >> 16) & 0xFF);
    m_Encode[8] = static_cast<unsigned char>((rawLat >> 24) & 0xFF);

    // Longitude (Bytes 9 à 12)
    m_Encode[9] = static_cast<unsigned char>(rawLon & 0xFF);
    m_Encode[10] = static_cast<unsigned char>((rawLon >> 8) & 0xFF);
    m_Encode[11] = static_cast<unsigned char>((rawLon >> 16) & 0xFF);
    m_Encode[12] = static_cast<unsigned char>((rawLon >> 24) & 0xFF);
}

void CPGN_CNMEA_129038::GenerateRandomData(double trueDeltaTime, double timeFactor)
{
    if (!m_pRouteur)
        return;

    double centerLat = 43.5800000;
    double centerLon = 7.1200000;
    if (m_pOwnShip)
    {
        centerLat = m_pOwnShip->getLatitude();
        centerLon = m_pOwnShip->getLongitude();
    }

    if (m_pRouteur->g_aisTargets.empty())
        m_pRouteur->InitAISTargets(centerLat, centerLon, 5, 10);

    const int action = rand() % 100;
    if (action < 16)
    {
        m_pRouteur->AddAISTarget(centerLat, centerLon, 20.0);
    }
    else if (action >= 84)
    {
        m_pRouteur->RemoveAISTarget();
    }

    m_pRouteur->UpdateAISTargets(trueDeltaTime * timeFactor);

    CRouteurEmulateurNMEA::sAISTarget target{};
    if (!m_pRouteur->GetNextAISTarget(target))
    {
        m_MMSI = 0;
        m_Latitude = centerLat;
        m_Longitude = centerLon;
        return;
    }

    m_MMSI = target.mmsi;
    m_Latitude = target.latitude;
    m_Longitude = target.longitude;
}
uint32_t CPGN_CNMEA_129038::getMMSI() 
{ 
    return m_MMSI; 
}
double CPGN_CNMEA_129038::getLatitude() 
{ 
    return m_Latitude; 
}
double CPGN_CNMEA_129038::getLongitude() 
{ 
    return m_Longitude; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
CPGN_CNMEA_130306::CPGN_CNMEA_130306(CRouteurEmulateurNMEA* pRouteur)
{
    m_Priority = 3;
    m_SourceAddress = 0x03;   // Adresse de l’anémomètre
    m_PGN = 130306;           // PGN Wind Data
    m_pRouteur = pRouteur;
    setHeader();

    // Valeurs initiales
    m_WindAngleDeg = BoundedRand(0, 360, 1);
    m_WindSpeedKnots = BoundedRand(0, 40, 1);

    m_Encode.resize(8, 0xFF);

    StartLoop(); // Démarre automatiquement le thread producteur
}

void CPGN_CNMEA_130306::GenerateRandomData(double dt, double timeFactor)
{
    // Variation lente du vent
    m_WindAngleDeg += ((rand() % 31) - 15) * 0.1; // +/- 1.5°
    if (m_WindAngleDeg < 0) m_WindAngleDeg += 360;
    if (m_WindAngleDeg >= 360) m_WindAngleDeg -= 360;

    // Variation de vitesse
    m_WindSpeedKnots += ((rand() % 21) - 10) * 0.1; // +/- 1 nœud
    if (m_WindSpeedKnots < 0) m_WindSpeedKnots = 0;
    if (m_WindSpeedKnots > 40) m_WindSpeedKnots = 40;
}

void CPGN_CNMEA_130306::encode()
{
    m_Encode.resize(8);

    // SID
    m_Encode[0] = 0x01;

    // Direction en radians * 10000
    double angleRad = m_WindAngleDeg * 3.14159265358979323846 / 180.0;
    uint16_t rawAngle = static_cast<uint16_t>(angleRad * 10000.0);

    m_Encode[1] = rawAngle & 0xFF;
    m_Encode[2] = (rawAngle >> 8) & 0xFF;

    // Vitesse en m/s * 100
    double speedMS = m_WindSpeedKnots * 0.514444; // conversion nœuds → m/s
    uint16_t rawSpeed = static_cast<uint16_t>(speedMS * 100.0);

    m_Encode[3] = rawSpeed & 0xFF;
    m_Encode[4] = (rawSpeed >> 8) & 0xFF;

    // Référence : 0 = apparent
    m_Encode[5] = 0x00;

    // Réservé
    m_Encode[6] = 0xFF;
    m_Encode[7] = 0xFF;
}

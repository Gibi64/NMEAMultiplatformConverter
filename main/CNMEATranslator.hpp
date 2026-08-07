#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <cstdint>
#include "CUDP_Broadcast_Server.hpp"
#include "CTimeUtils.hpp"
#include "InitLog.hpp"
#include <math.h>
class CRouteurEmulateurNMEA;
#ifdef _SERIALEMULATOR
#else
#include "SerialClient.h"
#endif       

class CCanFrame
{
public:
    uint32_t timestamp; // Timestamp in milliseconds
    uint32_t pgn;       // PGN (Parameter Group Number)
    uint8_t length;     // Length of the data
    std::vector<unsigned char> data; // Data bytes
};
class CCanClient
{
public:
    bool Init();
    bool Read(CCanFrame& frame);
};


class CNMEATranslator
{
    #define M_PI 3.14159265358979323846
public:
	// On definit toujours la structure sArgumentsEmulator pour l'emulateur, sinon on definit la structure sArgumentsSerial pour le port serie
	struct sArgumentsEmulator
        {
        CNMEATranslator * pTranslator;
        CRouteurEmulateurNMEA* pRouteur; // le routeur qui contient le FIFO qui sert d'INPUT
        int Timer_ms;
	};
#if defined(_WIN32) || defined(__linux__)
#if !defined(_SERIALEMULATOR)
        struct sArgumentsSerial
        {
            CNMEATranslator* pTranslator;
            CSerialClient* pSerial;
            int Timer_ms;
        };
#else
#endif
    #elif defined(_ESP32)
        struct sArgumentsCAN
        {
            CNMEATranslator* pTranslator;
            CCanClient* pCAN;
            int Timer_ms;
        };
#endif
    struct sUTCTime
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        int millisecond;
        bool empty()
        {
            return year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && millisecond == 0;
        }
    };
    struct sRMCData
    {
        double latitude;
        double longitude;
        double speedKnots;
        double courseOverGround;
        sUTCTime utcTime;
	};
    struct sNavDelta
    {
        double distanceMeters;   // distance entre les deux points
        double speedKnots;       // vitesse en nœuds
        double courseDegrees;    // cap en degrés (0 = Nord, 90 = Est)
    };
    struct sAISUpdate
    {
        uint32_t mmsi;
        double latitude;
        double longitude;
        uint64_t receivedMs;
    };
    struct sAISContact
    {
        uint32_t mmsi;
        double latitude;
        double longitude;
        uint64_t lastSeenMs;
        bool dirty;
    };

private:
    bool m_bStarted = false;
    std::vector<unsigned char> m_Received;
    sUTCTime m_UTCTime;
	CUDP_Broadcast_Server* m_pUDPServer;
    std::vector<sAISUpdate> m_AISPending;
    std::map<uint32_t, sAISContact> m_AISContacts;
    std::mutex m_AISPendingMutex;
    std::mutex m_AISContactsMutex;
public:
    std::map<int, std::string(*)(const std::vector<unsigned char> &, CNMEATranslator *)> m_MapPGN;
	/////////// Vrcteur de bufferisation pour le traitement des données RMC en particulier les donnees moyennes
    struct sRMCMeanData
    {
		int m_Count;
        sRMCData RMCMeanData;
    };
    struct sDBTMeanData
    {
        int m_Count;
        double DepthMeanData;
	};
    struct SMWVMeanData
    {
        int m_Count;
        double WindSpeed;
		double WindAngle;
	};
    sRMCMeanData m_RMCMeanData;
    CNMEATranslator(CUDP_Broadcast_Server* udpServer);
    static std::string DecodePGN_129025(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_129038(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_130306(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_130321(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_126992(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_128259(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_127250(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_127251(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string DecodePGN_128267(const std::vector<unsigned char> &Encoded, CNMEATranslator* pTranslator);
    static std::string CalculateNMEAChecksum(const std::string sentence);
    static std::string ToNMEA0183Coord(double deg, bool isLat);
    void StartLoop();
    static int GetDataCount(int pgn);
    struct sArgumentsUDP
    {
        CNMEATranslator* pTranslator;
        CUDP_Broadcast_Server* pUDPServer;
        int Timer_ms;
	};
    struct sArgumentsAIS
    {
        CNMEATranslator* pTranslator;
        CRouteurEmulateurNMEA* pRouteur;
        int Timer_ms;
        int StaleTimeout_ms;
    };
    static void LoopExternalReadData(void *Args);
    static void LoopAIS(void* Args);
    void EnqueueAISUpdate(uint32_t mmsi, double latitude, double longitude);
    void ProcessAISUpdates();
    void PurgeAISContacts(uint64_t nowMs, uint64_t staleTimeoutMs);
    std::vector<std::string> ConsumeAISMessages();
    size_t GetAISContactCount();
    static sUTCTime DecodeZDA(std::string zda);
    static uint64_t ToEpoch2001ms(const sUTCTime& t);
    static sNavDelta ComputeDelta(const sRMCData& p1, const sRMCData& p2);
    bool StackMeanNavData(sRMCData data);
    bool StackMeanDepthData(double depth);
    bool StackMeanWindData(double speed, double angle);
    std::string BuildRMC();
    std::string BuildDBT();
    std::string BuildMWV();
	static void LoopTCP_UDPSend(void* Args);
};

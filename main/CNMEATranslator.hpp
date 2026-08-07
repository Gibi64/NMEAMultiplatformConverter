#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <map>
#include <string>
#include "CUDP_Broadcast_Server.hpp"
#include "CTimeUtils.hpp"
#include "InitLog.hpp"
#include "CShip.hpp"
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
    using tPGNDecoder = std::string(*)(const std::vector<unsigned char>&, CNMEATranslator*);
    struct sPGNHandler
    {
        int dataCount;
        tPGNDecoder decoder;
    };

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
private:
    bool m_bStarted = false;
    std::vector<unsigned char> m_Received;
    CUDP_Broadcast_Server* m_pUDPServer;
    CMyShip m_myShip;
    COtherBoats m_otherBoats;
public:
    std::map<int, sPGNHandler> m_MapPGN;
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
    int GetDataCount(int pgn) const;
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
	static void LoopTCP_UDPSend(void* Args);
};

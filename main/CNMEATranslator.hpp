#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <map>
#include <string>
#include "CUDP_Broadcast_Server.hpp"
#include "CTimeUtils.hpp"
#include "InitLog.hpp"
#include <math.h>
#include <mutex>
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
private:
	std::mutex m_mutex_MapPGN; 
public:
	std::mutex* GetMutex_MapPGN() { return &m_mutex_MapPGN; }
    CNMEATranslator(CUDP_Broadcast_Server* udpServer);


    struct sCumulativeResult
    {
        double m_Data[4] = { 0,0,0,0 };
        uint32_t m_Count = 0;
        uint64_t StartTime = 0;
        uint64_t LastTime = 0;
		bool ReadyToSend = false;
        double& operator[](size_t index) { return m_Data[index]; }
    };

    #define M_PI 3.14159265358979323846
private:
    std::map<long,sCumulativeResult*> m_Map_Result_Cumulative;
public:
    sCumulativeResult* GetCumulativeResult(long PGN_Number)
    {
        auto it = m_Map_Result_Cumulative.find(PGN_Number);
        if (it != m_Map_Result_Cumulative.end())
        {
            return it->second;
        }
        else
        {
            sCumulativeResult* pResult = new sCumulativeResult();
            m_Map_Result_Cumulative[PGN_Number] = pResult;
            return pResult;
        }
    }
    ////////////////////// Map des AIS ////////////////////////////////////////:
    struct sAIS
    {
    public:
        sCumulativeResult CumulativeResult;
        long long LastSent;
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
private:
	std::map<long, sAIS*> m_Map_AIS;
public:
    std::map<long, sAIS*>* Get_Map_AIS()
    {
        return &m_Map_AIS;
    }
    sAIS* GetAISResult(long MMSI_Number)
    {
        auto it = m_Map_AIS.find(MMSI_Number);
        if (it != m_Map_AIS.end())
        {
            return it->second;
        }
        else
        {
            sAIS* pResult = new sAIS();
            m_Map_AIS[MMSI_Number] = pResult;
            return pResult;
        }
	}
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
public:
    CUDP_Broadcast_Server *GetUDP_Server()
    {
        return m_pUDPServer;
	}
    typedef void(*decode)(CNMEATranslator*, const std::vector<unsigned char>&);
    typedef std::string(*Translate)(sCumulativeResult *);

    struct sPGNHandler {
        decode decoder;
		Translate translator;
        int dataCount;
        std::string KeywordNMEA;
    };

    std::map<int, sPGNHandler> m_MapPGN;
    static std::string ToNMEA0183Coord(double deg, bool isLat);
	static std::string CalculateNMEAChecksum(const std::string& sentence);
    void StartLoop();
    int GetDataCount(int pgn) const;
    ///////////////////////////////////////////////////////////////////

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
    struct sWindData
    {
        double speedMs = 0.0;
        double angleRad = 0.0;
    };

    static void LoopExternalReadData(void *Args);
	static void LoopTCP_UDPSend(void* Args);
};

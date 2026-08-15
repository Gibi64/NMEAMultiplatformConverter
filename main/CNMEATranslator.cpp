#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#include <math>
#endif
#include "CRouteurEmulateurNMEA.hpp"
#include "CNMEATranslator.hpp"
#include "InitLog.hpp"
#include "Decode.hpp"
#include "Translate.hpp"
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

    m_MapPGN[129025] = { &CDecode::DecodePGN129025 ,&CTranslate::TranlateRMC ,8,"RMC"};
    m_MapPGN[128267] = { &CDecode::DecodePGN128267,&CTranslate::TranslateDBT,8,"DBT"};
    m_MapPGN[129038] = { &CDecode::DecodePGN129038,nullptr,28,""};// Pas de translate ?
    m_MapPGN[130306] = { &CDecode::DecodePGN130306,&CTranslate::TranslateMWV,8,"MWV"};
    //m_MapPGN[128259] = { &CDecode::DecodePGN128259,&CTranslate::TranslateHDT,8,"HDT"};
    //m_MapPGN[127250] = { &CDecode::DecodePGN127250,&CTranslate::TranslateHDT,8,"HDT"};
}


std::string CNMEATranslator::CalculateNMEAChecksum(const std::string& sentence)
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

std::string CNMEATranslator::ToNMEA0183Coord(double deg, bool isLat)
{
    char hemi = (isLat ? (deg >= 0 ? 'N' : 'S') : (deg >= 0 ? 'E' : 'W'));
    deg = deg>0 ? deg : -deg;

    int d = static_cast<int>(deg);
    double m = (deg - d) * 60.0;

    char buf[32];
    snprintf(buf, sizeof(buf), isLat ? "%02d%07.4f,%c" : "%03d%07.4f,%c", d, m, hemi);
    return std::string(buf);
}

void CNMEATranslator::StartLoop()
{
    m_bStarted = true;
}

int CNMEATranslator::GetDataCount(int pgn) const
{
    auto it = m_MapPGN.find(pgn);
    if (it == m_MapPGN.end())
        return 0;
    return it->second.dataCount;
}

void CNMEATranslator::LoopExternalReadData(void* Args)
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

    if (!pTransltator->m_bStarted)
        return;

    int CurrentPgn = 0;
    int HeaderSize = 4;
    int nbrOfBytes = 0;

    for (;;)
    {
#if defined(_SERIALEMULATOR)
        sArgumentsEmulator* pArgs = static_cast<sArgumentsEmulator*>(Args);
        CNMEATranslator* pTranslator = pArgs->pTranslator;
        CRouteurEmulateurNMEA* pRouteur = pArgs->pRouteur;
        {
            std::lock_guard<std::mutex> lock(pRouteur->GetFIFOMutex());

            if (!pRouteur->g_fifo_Send.empty())
            {
                auto& frame = pRouteur->g_fifo_Send.front();
                nbrOfBytes = static_cast<int>(frame.size());
                for (int i = 0; i < nbrOfBytes; ++i)
                    pTranslator->m_Received.push_back(frame[static_cast<size_t>(i)]);
                pRouteur->g_fifo_Send.erase(pRouteur->g_fifo_Send.begin());
            }
        }
        auto bufferSize = pTranslator->m_Received.size();
#else
#if defined(_WIN32) || defined(__linux__)
        if (pSerial->GetSerialHandle() == INVALID_HANDLE_VALUE)
            break;

        nbrOfBytes = pSerial->getNbrOfBytes();
        if (nbrOfBytes)
        {
            for (int i = 0; i < nbrOfBytes; ++i)
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
                pTransltator->m_Received.push_back(frame.data[static_cast<size_t>(i)]);
        }
        auto bufferSize = pTransltator->m_Received.size();
#endif
#endif

        for (;;)
        {
            if (!CurrentPgn)
            {
                if (bufferSize < HeaderSize)
                {
                    CTimeUtils::CPUSleep(1);   // ← indispensable
                    break;
                }
                uint32_t header = (static_cast<uint32_t>(pTranslator->m_Received[0]) << 24) |
                    (static_cast<uint32_t>(pTranslator->m_Received[1]) << 16) |
                    (static_cast<uint32_t>(pTranslator->m_Received[2]) << 8) |
                    static_cast<uint32_t>(pTranslator->m_Received[3]);
                CurrentPgn = (header >> 8) & 0x3FFFF;
                pTranslator->m_Received.erase(pTranslator->m_Received.begin(), pTranslator->m_Received.begin() + HeaderSize);
                bufferSize = pTranslator->m_Received.size();
            }

            auto it = pTranslator->m_MapPGN.find(CurrentPgn);
            if (it == pTranslator->m_MapPGN.end())
            {
                CurrentPgn = 0;
                continue;
            }

            const int dataCount = it->second.dataCount;
            if (dataCount <= 0)
            {
                CurrentPgn = 0;
                continue;
            }

            if (bufferSize < static_cast<size_t>(dataCount))
                break;
            auto decodeFunction = it->second.decoder;
            if (decodeFunction != nullptr)
            {
                std::lock_guard<std::mutex> lock(*pTranslator->GetMutex_MapPGN());
                decodeFunction(pTranslator, std::vector<unsigned char>(pTranslator->m_Received.begin(), pTranslator->m_Received.begin() + dataCount));
                pTranslator->m_Received.erase(pTranslator->m_Received.begin(), pTranslator->m_Received.begin() + dataCount);
            
            }
            CurrentPgn = 0;
            bufferSize = pTranslator->m_Received.size();
        }
    }
    CTimeUtils::CPUSleep(2);

}

void CNMEATranslator::LoopTCP_UDPSend(void* Args)
{
    auto pArgs = static_cast<sArgumentsUDP*>(Args);
    CNMEATranslator* pTranslator = pArgs->pTranslator;
    if (!pTranslator->m_bStarted)
        return;

    unsigned long long startTime = CTimeUtils::GetMs();
    for (;;)
    {
        if (CTimeUtils::GetMs() - startTime > static_cast<unsigned long long>(pArgs->Timer_ms))
        {
            for (auto it = pTranslator->m_MapPGN.begin(); it != pTranslator->m_MapPGN.end(); it++)
            {
             
                std::lock_guard<std::mutex> lock(*pTranslator->GetMutex_MapPGN());
                auto R = pTranslator->GetCumulativeResult(it->first);
                if (R && R->ReadyToSend)
                {
                    auto Translate = it->second.translator;
                    if (Translate)
                    {
                        std::string s = Translate(R);
                        pTranslator->GetUDP_Server()->send(s);
                        write_log("Sending " + it->second.KeywordNMEA + " : " + s + "\n");
                        R->ReadyToSend = false;
                        R->m_Count = 1;
                    }
                }
            }
            startTime = CTimeUtils::GetMs();

        }
        CTimeUtils::CPUSleep(2);

    }

}


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

    m_MapPGN[129025] = { 8, &CNMEATranslator::DecodePGN_129025 };
    m_MapPGN[128267] = { 8, &CNMEATranslator::DecodePGN_128267 };
    m_MapPGN[129038] = { 28, &CNMEATranslator::DecodePGN_129038 };
    m_MapPGN[130306] = { 8, &CNMEATranslator::DecodePGN_130306 };
    m_MapPGN[126992] = { 8, &CNMEATranslator::DecodePGN_126992 };
    m_MapPGN[128259] = { 8, &CNMEATranslator::DecodePGN_128259 };
    m_MapPGN[127250] = { 8, &CNMEATranslator::DecodePGN_127250 };
}

std::string CNMEATranslator::DecodePGN_129025(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    pTranslator->m_myShip.DecodePGN129025(encoded);
    return "";
}

std::string CNMEATranslator::DecodePGN_129038(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    return pTranslator->m_otherBoats.DecodePGN129038(encoded);
}

std::string CNMEATranslator::DecodePGN_130306(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    pTranslator->m_myShip.DecodePGN130306(encoded);
    return "";
}

std::string CNMEATranslator::DecodePGN_126992(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    pTranslator->m_myShip.DecodePGN126992(encoded);
    return "";
}

std::string CNMEATranslator::DecodePGN_128259(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    pTranslator->m_myShip.DecodePGN128259(encoded);
    return "";
}

std::string CNMEATranslator::DecodePGN_127250(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    pTranslator->m_myShip.DecodePGN127250(encoded);
    return "";
}

std::string CNMEATranslator::DecodePGN_128267(const std::vector<unsigned char>& encoded, CNMEATranslator* pTranslator)
{
    pTranslator->m_myShip.DecodePGN128267(encoded);
    return "";
}

std::string CNMEATranslator::CalculateNMEAChecksum(const std::string sentence)
{
    return CShip::CalculateNMEAChecksum(sentence);
}

std::string CNMEATranslator::ToNMEA0183Coord(double deg, bool isLat)
{
    return CShip::ToNMEA0183Coord(deg, isLat);
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
        CNMEATranslator* pTransltator = pArgs->pTranslator;
        CRouteurEmulateurNMEA* pRouteur = pArgs->pRouteur;
        {
            std::lock_guard<std::mutex> lock(pRouteur->GetFIFOMutex());

            if (!pRouteur->g_fifo_Send.empty())
            {
                auto& frame = pRouteur->g_fifo_Send.front();
                nbrOfBytes = static_cast<int>(frame.size());
                for (int i = 0; i < nbrOfBytes; ++i)
                    pTransltator->m_Received.push_back(frame[static_cast<size_t>(i)]);
                pRouteur->g_fifo_Send.erase(pRouteur->g_fifo_Send.begin());
            }
        }
        auto bufferSize = pTransltator->m_Received.size();
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
                if (bufferSize < static_cast<size_t>(HeaderSize))
                    break;

                uint32_t header = (static_cast<uint32_t>(pTransltator->m_Received[0]) << 24) |
                    (static_cast<uint32_t>(pTransltator->m_Received[1]) << 16) |
                    (static_cast<uint32_t>(pTransltator->m_Received[2]) << 8) |
                    static_cast<uint32_t>(pTransltator->m_Received[3]);
                CurrentPgn = (header >> 8) & 0x3FFFF;
                pTransltator->m_Received.erase(pTransltator->m_Received.begin(), pTransltator->m_Received.begin() + HeaderSize);
                bufferSize = pTransltator->m_Received.size();
            }

            auto it = pTransltator->m_MapPGN.find(CurrentPgn);
            if (it == pTransltator->m_MapPGN.end())
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

            std::string decodedMessage = it->second.decoder(
                std::vector<unsigned char>(pTransltator->m_Received.begin(), pTransltator->m_Received.begin() + dataCount),
                pTransltator);

            pTransltator->m_Received.erase(pTransltator->m_Received.begin(), pTransltator->m_Received.begin() + dataCount);
            CurrentPgn = 0;
            bufferSize = pTransltator->m_Received.size();
        }
    }
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
            std::string rmc = pTranslator->m_myShip.BuildRMC();
            if (!rmc.empty())
            {
                write_log("Sending RMC: " + rmc + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(rmc);
            }

            std::string dbt = pTranslator->m_myShip.BuildDBT();
            if (!dbt.empty())
            {
                write_log("Sending DBT: " + dbt + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(dbt);
            }

            std::string mwv = pTranslator->m_myShip.BuildMWV();
            if (!mwv.empty())
            {
                write_log("Sending MWV: " + mwv + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(mwv);
            }

            std::string vtg = pTranslator->m_myShip.BuildVTG();
            if (!vtg.empty())
            {
                write_log("Sending VTG: " + vtg + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(vtg);
            }

            std::string gga = pTranslator->m_myShip.BuildGGA();
            if (!gga.empty())
            {
                write_log("Sending GGA: " + gga + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(gga);
            }

            std::string hdt = pTranslator->m_myShip.BuildHDT();
            if (!hdt.empty())
            {
                write_log("Sending HDT: " + hdt + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(hdt);
            }

            std::string hdg = pTranslator->m_myShip.BuildHDG();
            if (!hdg.empty())
            {
                write_log("Sending HDG: " + hdg + "\n");
                if (pTranslator->m_pUDPServer)
                    pTranslator->m_pUDPServer->send(hdg);
            }

            startTime = CTimeUtils::GetMs();
        }

        CTimeUtils::CPUSleep(2);
    }
}

void CNMEATranslator::LoopAIS(void* Args)
{
    auto pArgs = static_cast<sArgumentsAIS*>(Args);
    CNMEATranslator* pTranslator = pArgs->pTranslator;
    if (!pTranslator->m_bStarted)
        return;

    uint64_t lastLogTimeMs = CTimeUtils::GetMs();
    for (;;)
    {
        double ownLatitude = 0.0;
        double ownLongitude = 0.0;
        const bool hasOwnPosition = pTranslator->m_myShip.TryGetCurrentPosition(ownLatitude, ownLongitude);

        pTranslator->m_otherBoats.ProcessAISUpdates();
        pTranslator->m_otherBoats.PurgeAISContacts(
            CTimeUtils::GetMs(),
            static_cast<uint64_t>(pArgs->StaleTimeout_ms),
            ownLatitude,
            ownLongitude,
            hasOwnPosition);

        if (pTranslator->m_pUDPServer)
        {
            auto aisMessages = pTranslator->m_otherBoats.ConsumeAISMessages(ownLatitude, ownLongitude, hasOwnPosition);
            for (const auto& message : aisMessages)
            {
                write_log("Sending AIS: " + message + "\n");
                pTranslator->m_pUDPServer->send(message);
            }
        }

        const uint64_t nowMs = CTimeUtils::GetMs();
        if (nowMs - lastLogTimeMs >= 1000)
        {
            const size_t targetCount = pTranslator->m_otherBoats.GetAISContactCount();
            write_log("Nombre de cibles AIS : " + std::to_string(targetCount) + "\n");
            lastLogTimeMs = nowMs;
        }

        CTimeUtils::CPUSleep(pArgs->Timer_ms > 0 ? pArgs->Timer_ms : 20);
    }
}

#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif

#include "CRouteurEmulateurNMEA.hpp"
#include "CTimeUtils.hpp"
#include "InitLog.hpp"

void CRouteurEmulateurNMEA::PushFIFO(const unsigned char* data, size_t size)
{   
    std::lock_guard<std::mutex> lock(m_fifoMutex);
    g_fifo_Send.emplace_back(data, data + size);
}

void CRouteurEmulateurNMEA::LoopFIFOThreadServer(void* Args)
{
#ifndef _SERIALEMULATOR
    sArgumentsLoopRouteur* args = reinterpret_cast<sArgumentsLoopRouteur*>(Args);
    CRouteurEmulateurNMEA* pRouteur = reinterpret_cast<CRouteurEmulateurNMEA*>(args->pRouteur);
    CSerialClient* client = args->pClient;
#else
    sArgumentsLoopRouteur* args = reinterpret_cast<sArgumentsLoopRouteur*>(Args);
    CRouteurEmulateurNMEA* pRouteur = reinterpret_cast<CRouteurEmulateurNMEA * >(args->pRouteur) ;
#endif
    std::vector<unsigned char> dataToSend;
    write_log("Starting LoopFIFOThreadServer");
    while (!pRouteur->g_bStopThread.load())
    {
        { // Zone prot�g�e pour acc�der � g_fifo_Send, ces {} sont importantes pour limiter la port�e du lock
            std::lock_guard<std::mutex> lock(pRouteur->m_fifoMutex);
#ifndef _SERIALEMULATOR
            if (!pRouteur->g_fifo_Send.empty() && client->GetSerialHandle() != INVALID_HANDLE_VALUE)
            {
                dataToSend = std::move(pRouteur->g_fifo_Send.front());
                pRouteur->g_fifo_Send.erase(pRouteur->g_fifo_Send.begin());
                client->sendArray(reinterpret_cast<char*>(dataToSend.data()), dataToSend.size());
#else
            if (!pRouteur->g_fifo_Send.empty())
            {
				pRouteur->CheckAndPurgeExtraRecords(); // On purge les anciens enregistrements si le FIFO est trop grand

#endif

            }
        }
    // On rend la main 2ms � l'OS 
    CTimeUtils::CPUSleep(2);
    }
}
unsigned long CRouteurEmulateurNMEA::GetFIFOSize()
{
    //std::lock_guard<std::mutex> lock(m_fifoMutex);

    unsigned long Total_Size = 0;
    for (auto it = g_fifo_Send.begin(); it != g_fifo_Send.end(); it++)
    {
        Total_Size += it->size();
    }
    return Total_Size;
}
void CRouteurEmulateurNMEA::CheckAndPurgeExtraRecords()
{
    //std::lock_guard<std::mutex> lock(m_fifoMutex);

    unsigned long fifoSize = GetFIFOSize();
    if (fifoSize < 4000L) return;


    while (fifoSize >= 3000L && !g_fifo_Send.empty())
    {
        fifoSize -= g_fifo_Send.front().size();
        g_fifo_Send.erase(g_fifo_Send.begin());
    }
	write_log("CRouteurEmulateurNMEA::CheckAndPurgeExtraRecords: Purged records to reduce FIFO size. New size: " + std::to_string(fifoSize) + "\n");
}
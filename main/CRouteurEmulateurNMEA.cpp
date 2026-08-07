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
                dataToSend = std::move(pRouteur->g_fifo_Send.front());
                // On limite la taille de fifo_Send � 1000 �l�ments pour �viter une croissance infinie
                // On ne garde que les 1000 premiers �l�ments, les autres sont supprim�s
                if (pRouteur->g_fifo_Send.size() > 1000)
                {
                    pRouteur->g_fifo_Send.erase(pRouteur->g_fifo_Send.begin() + 1000, pRouteur->g_fifo_Send.begin() + (pRouteur->g_fifo_Send.size()));
                }
                // C'est le Translator qui videra le FIFO, ici on ne fait que le limiter � 1000 �l�ments, on ne l'efface pas encore

#endif

            }
        }
    // On rend la main 2ms � l'OS 
    CTimeUtils::CPUSleep(2);
    }
}

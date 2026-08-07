#pragma once
#include <mutex>
#include <iostream>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#if defined(_SERIALEMULATOR)
struct sArgumentsLoopRouteur
{
    void* pRouteur;
};

#else
#include "serialclient.h"
struct sArgumentsLoopRouteur
{
    void* pRouteur;
    CSerialClient* pClient;
};

#if defined(_ESP32)
#endif

#endif
#include<vector>
#include <map>
#include <atomic>
class CRouteurEmulateurNMEA
{
private:
    std::mutex m_fifoMutex;

public:
    std::mutex& GetMutex() { return m_fifoMutex; } // Compatibilite: mutex FIFO
    std::mutex& GetFIFOMutex() { return m_fifoMutex; }
    std::vector<std::vector<unsigned char>> g_fifo_Send; // Global buffer to store BYTES to send
    std::atomic<bool> g_bStopThread{ false }; // Global flag to control the thread
    void PushFIFO(const unsigned char* data, size_t size);
    static void LoopFIFOThreadServer(void* Args);
};


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
    std::mutex m_aisMutex;
    size_t m_AISCursor = 0;

public:
    struct sAISTarget
    {
        uint32_t mmsi;
        double latitude;
        double longitude;
        double headingDeg;
        double speedKnots;
    };

public:
    std::mutex& GetMutex() { return m_fifoMutex; } // Compatibilite: mutex FIFO
    std::mutex& GetFIFOMutex() { return m_fifoMutex; }
    std::mutex& GetAISMutex() { return m_aisMutex; }
    std::vector<std::vector<unsigned char>> g_fifo_Send; // Global buffer to store BYTES to send
    std::vector<sAISTarget> g_aisTargets;
    std::atomic<bool> g_bStopThread{ false }; // Global flag to control the thread
    void PushFIFO(const unsigned char* data, size_t size);
    void InitAISTargets(double centerLat, double centerLon, int minCount = 5, int maxCount = 10);
    void AddAISTarget(double centerLat, double centerLon, double radiusNm = 20.0);
    void RemoveAISTarget();
    size_t GetAISTargetCount();
    void UpdateAISTargets(double deltaTimeSec = 0.1);
    bool GetNextAISTarget(sAISTarget& target);
    static void LoopFIFOThreadServer(void* Args);
};


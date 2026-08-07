#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif

#include "CRouteurEmulateurNMEA.hpp"
#include "CTimeUtils.hpp"
#include "InitLog.hpp"
#include <cmath>
#include <cstdlib>

namespace
{
double RandRange(double minValue, double maxValue)
{
    return minValue + (static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) * (maxValue - minValue);
}

CRouteurEmulateurNMEA::sAISTarget BuildRandomAISTarget(double centerLat, double centerLon, double radiusNm, size_t sequence)
{
    const double latDeltaDeg = radiusNm / 60.0;
    const double cosLat = cos(centerLat * 3.14159265358979323846 / 180.0);
    const double lonDeltaDeg = radiusNm / (60.0 * (fabs(cosLat) < 1e-6 ? 1e-6 : cosLat));
    const double latMin = centerLat - latDeltaDeg;
    const double latMax = centerLat + latDeltaDeg;
    const double lonMin = centerLon - lonDeltaDeg;
    const double lonMax = centerLon + lonDeltaDeg;

    CRouteurEmulateurNMEA::sAISTarget target{};
    target.mmsi = static_cast<uint32_t>(227000000 + static_cast<uint32_t>(sequence * 100) + static_cast<uint32_t>(rand() % 99));
    target.latitude = RandRange(latMin, latMax);
    target.longitude = RandRange(lonMin, lonMax);
    target.headingDeg = RandRange(0.0, 360.0);
    target.speedKnots = RandRange(0.0, 30.0);
    return target;
}
}

void CRouteurEmulateurNMEA::PushFIFO(const unsigned char* data, size_t size)
{   
    std::lock_guard<std::mutex> lock(m_fifoMutex);
    g_fifo_Send.emplace_back(data, data + size);
}

void CRouteurEmulateurNMEA::InitAISTargets(double centerLat, double centerLon, int minCount, int maxCount)
{
    std::lock_guard<std::mutex> lock(m_aisMutex);
    if (!g_aisTargets.empty())
        return;

    const double InitialAisRadiusNm = 20.0;

    const int safeMin = minCount < 1 ? 1 : minCount;
    const int safeMax = maxCount < safeMin ? safeMin : maxCount;
    const int targetCount = safeMin + (rand() % (safeMax - safeMin + 1));

    g_aisTargets.reserve(static_cast<size_t>(targetCount));
    for (int i = 0; i < targetCount; ++i)
        g_aisTargets.push_back(BuildRandomAISTarget(centerLat, centerLon, InitialAisRadiusNm, static_cast<size_t>(i)));
    m_AISCursor = 0;
}

void CRouteurEmulateurNMEA::AddAISTarget(double centerLat, double centerLon, double radiusNm)
{
    std::lock_guard<std::mutex> lock(m_aisMutex);
    g_aisTargets.push_back(BuildRandomAISTarget(centerLat, centerLon, radiusNm, g_aisTargets.size()));
}

void CRouteurEmulateurNMEA::RemoveAISTarget()
{
    std::lock_guard<std::mutex> lock(m_aisMutex);
    if (g_aisTargets.empty())
        return;

    const size_t index = static_cast<size_t>(rand() % g_aisTargets.size());
    g_aisTargets.erase(g_aisTargets.begin() + static_cast<std::ptrdiff_t>(index));
    if (m_AISCursor > g_aisTargets.size())
        m_AISCursor = g_aisTargets.size();
}

size_t CRouteurEmulateurNMEA::GetAISTargetCount()
{
    std::lock_guard<std::mutex> lock(m_aisMutex);
    return g_aisTargets.size();
}

void CRouteurEmulateurNMEA::UpdateAISTargets(double deltaTimeSec)
{
    std::lock_guard<std::mutex> lock(m_aisMutex);
    if (g_aisTargets.empty())
        return;

    for (auto& target : g_aisTargets)
    {
        double nextLat = target.latitude;
        double nextLon = target.longitude;

        const double speedDegPerSec = (target.speedKnots / 60.0) / 3600.0;
        const double headingRad = target.headingDeg * 3.14159265358979323846 / 180.0;
        nextLat += speedDegPerSec * cos(headingRad) * deltaTimeSec;
        nextLon += speedDegPerSec * sin(headingRad) * deltaTimeSec;

        target.latitude = nextLat;
        target.longitude = nextLon;
    }
}

bool CRouteurEmulateurNMEA::GetNextAISTarget(sAISTarget& target)
{
    std::lock_guard<std::mutex> lock(m_aisMutex);
    if (g_aisTargets.empty())
        return false;

    if (m_AISCursor >= g_aisTargets.size())
        m_AISCursor = 0;

    target = g_aisTargets[m_AISCursor];
    ++m_AISCursor;
    return true;
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

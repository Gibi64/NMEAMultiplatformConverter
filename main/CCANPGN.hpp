#pragma once
#include <vector>
#include <string>
#include <atomic>
class CRouteurEmulateurNMEA;
class CLaunchThread;
#if defined(_WIN32)
#include <winsock2.h>
#include <sysinfoapi.h>
#endif
#include "CTimeUtils.hpp"
class CNMEACAN
{
protected:
    uint32_t m_Header;
    CLaunchThread* m_pLauchThread = NULL;
    std::vector<unsigned char> m_Encode;
    int m_Priority;
    int m_PGN;
    int m_SourceAddress;
    CRouteurEmulateurNMEA* m_pRouteur = NULL;
    bool m_bStarted = false; // Flag to control the loop

public:
    virtual ~CNMEACAN();
    static double BoundedRand(double range_min, double range_max, int digit);
    static std::string ToNMEA0183Coord(double deg, bool isLat);

    void StartLoop();
    uint32_t getHeader();
    void setHeader();
    virtual void encode() = 0;
    virtual void GenerateRandomData(double trueDeltaTime, double timeFactor) = 0;
    static void LoopProducer(void *pArgs);
    virtual void send();

};
class CPGN_CNMEA_128259 : public CNMEACAN
{
public:
    double m_SpeedKnots;

    CPGN_CNMEA_128259(CRouteurEmulateurNMEA* pRouteur);
    virtual void GenerateRandomData(double dt, double tf) override;
    virtual void encode() override;
    virtual void send() override;
    void setSpeed(double knots);
};
class CPGN_CNMEA_127250 : public CNMEACAN
{
public:
    double m_HeadingDeg;
    CPGN_CNMEA_127250(CRouteurEmulateurNMEA* pRouteur);
    virtual void GenerateRandomData(double dt, double tf) override;
    virtual void encode() override;
    virtual void send() override;
    void setHeading(double heading);
};
class CPGN_CNMEA_126992 : public CNMEACAN
{
private:
    uint32_t m_Date;   // jours depuis 1970-01-01
    uint32_t m_Time;   // millisecondes depuis minuit

public:
    CPGN_CNMEA_126992(CRouteurEmulateurNMEA* pRouteur);
    virtual void GenerateRandomData(double dt, double tf) override;
    virtual void encode() override;

};

class CPGN_CNMEA_129025 : public CNMEACAN
{
    // class for PGN 129025 - Position, Rapid Update
private:
    double m_Latitude;  // En degrés décimaux (ex: 43.5800000)
    double m_Longitude; // En degrés décimaux (ex: 7.1200000)
    double m_SpeedKnots; // Vitesse en nœuds (ex: 5.0)
    double m_HeadingDeg; // Cap en degrés (ex: 90.0)
    int m_HeadingTimerAcc = 0; // Compteur pour le changement de cap
    int m_SpeedTimerAcc = 0; // Compteur pour le changement de vitesse
    CPGN_CNMEA_128259* m_pSpeed;
    CPGN_CNMEA_127250* m_pHeading;
    CPGN_CNMEA_126992* m_pTime;
public:
    CPGN_CNMEA_129025(CRouteurEmulateurNMEA* pRouteur);
    virtual void GenerateRandomData(double trueDeltaTime, double timeFactor);
    ~CPGN_CNMEA_129025();
    virtual void encode() override;
    double getLatitude();
    double getLongitude();
};
class CPGN_CNMEA_128267 : public CNMEACAN
{
private:
    double m_DepthMeters; // Profondeur en mètres (ex: 12.5m)

public:
    CPGN_CNMEA_128267(CRouteurEmulateurNMEA* pRouteur);
    void setDepth(double depthMeters);
    virtual void encode() override;
    double getDepth() ;
    void GenerateRandomData(double trueDeltaTime, double timeFactor = 1) override;
};
class CPGN_CNMEA_129038 : public CNMEACAN
{
private:
    uint32_t m_MMSI;
    double m_Latitude;
    double m_Longitude;

public:
    CPGN_CNMEA_129038(CRouteurEmulateurNMEA* pRouteur);
    void setTargetData(uint32_t mmsi, double latitude, double longitude);
    virtual void encode() override;
    virtual void GenerateRandomData(double trueDeltaTime, double timeFactor) override;
    uint32_t getMMSI();
    double getLatitude();
    double getLongitude();
};
class CPGN_CNMEA_130306 : public CNMEACAN
{
private:
    double m_WindAngleDeg;   // 0 à 360°
    double m_WindSpeedKnots; // 0 à 40 nœuds

public:
    CPGN_CNMEA_130306(CRouteurEmulateurNMEA* pRouteur);
    virtual void GenerateRandomData(double dt, double timeFactor) override;
    virtual void encode() override;
};
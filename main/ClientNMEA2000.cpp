// ClientNMEA2000.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//
#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif
#include "CNMEATranslator.hpp"
#include "CLaunchThread.hpp"
#include "CUDP_Broadcast_Server.hpp"
#include "InitLog.hpp"
#include "CRouteurEmulateurNMEA.hpp"
#include "CCANPGN.hpp"

#ifndef _TIMEFACTOR
#define _TIMEFACTOR 1.0
#endif

void MainLoop(void*)
{
    for(;;)
    {
       CTimeUtils::CPUSleep(2);
    }
}

#if defined(_ESP32)
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"


void InitWiFi_AP()
{
    // Initialisation NVS (obligatoire pour le WiFi)
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    // Crée l'interface WiFi AP
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Configuration du point d'accès
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, "ESP32-NMEA");
    wifi_config.ap.ssid_len = strlen("ESP32-NMEA");
    wifi_config.ap.channel = 1;
    strcpy((char*)wifi_config.ap.password, "12345678");
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    // Active le mode AP
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();

    printf("WiFi AP actif\n");
    printf("SSID : ESP32-NMEA\n");
    printf("IP ESP32 : 192.168.4.1\n");
    printf("Broadcast : 192.168.4.255\n");
}
extern "C" void app_main(void)
{
    InitWiFi_AP();

#else
int main()
{
        // Port serie pour l'emulateur windows
	CSerialClient Serial;
	Serial.connect((char*)"COM1", 9600, 'N', 8, 1);

#endif
    InitLog();
	CUDP_Broadcast_Server udpServer(10110);
    CNMEATranslator Translator(&udpServer);

    Translator.StartLoop();
    CNMEATranslator::sArgumentsUDP ArgsUDP;
    CNMEATranslator::sArgumentsAIS ArgsAIS;

    /////////////////////////////////////////////////////////////////////////////////////////////////
#if defined (_SERIALEMULATOR)
    // _TIMEFACTOR est fixe a la compilation (ex: 5.0 = 5x plus rapide)
    const double emulatorTimeFactor = static_cast<double>(_TIMEFACTOR);
    CNMEACAN::SetTimeFactor(emulatorTimeFactor);
    write_log("Emulator _TIMEFACTOR: " + std::to_string(emulatorTimeFactor) + "\n");

    CRouteurEmulateurNMEA Routeur;
    CPGN_CNMEA_129025 Position(&Routeur);
    CPGN_CNMEA_128267 Depth(&Routeur);
    CPGN_CNMEA_129038 AIS(&Routeur);

    CNMEATranslator::sArgumentsEmulator ArgsExternalDataInput;
    ArgsExternalDataInput.pTranslator = &Translator;
    ArgsExternalDataInput.pRouteur = &Routeur;
    ArgsExternalDataInput.Timer_ms = 2;// NOT USED

#else
#if defined(_WIN32) || defined(__linux__)
    CNMEATranslator::sArgumentsSerial ArgsExternalDataInput;
	ArgsExternalDataInput.pTranslator = &Translator;
	ArgsExternalDataInput.pSerial = &Serial;
	ArgsExternalDataInput.Timer_ms = 2;// NOT USED
#elif defined(_ESP32)
    CCanClient CAN;
    CAN.Init();
    CNMEATranslator::sArgumentsCAN ArgsExternalDataInput;
    ArgsExternalDataInput.pTranslator = &Translator;
    ArgsExternalDataInput.pCAN = &CAN;
    ArgsExternalDataInput.Timer_ms = 2;// NOT USED

#endif
#endif
    CLaunchThread ThreadExternalDataInput(&CNMEATranslator::LoopExternalReadData, &ArgsExternalDataInput );
	ArgsUDP.pTranslator = &Translator;
	ArgsUDP.pUDPServer = &udpServer;
    ArgsUDP.Timer_ms = 1000;
    CLaunchThread ThreadTCP(&CNMEATranslator::LoopTCP_UDPSend, &ArgsUDP );
    ArgsAIS.pTranslator = &Translator;
    ArgsAIS.pRouteur = nullptr;
    ArgsAIS.Timer_ms = 20;
    ArgsAIS.StaleTimeout_ms = 15000;
#if defined (_SERIALEMULATOR)
    ArgsAIS.pRouteur = &Routeur;
#endif
    CLaunchThread ThreadAIS(&CNMEATranslator::LoopAIS, &ArgsAIS);
#if defined (_SERIALEMULATOR)
	// On lance une thread FIFO pour générer les données NMEA vers le FIFO à haute frequence (10Hz)
    //
	sArgumentsLoopRouteur ArgsRouteur;
	ArgsRouteur.pRouteur = &Routeur;
	CLaunchThread ThreadFIFO(&CRouteurEmulateurNMEA::LoopFIFOThreadServer, &ArgsRouteur);
#endif
    CLaunchThread ThreadMainLoop(&MainLoop, nullptr);
    #if defined(_WIN32) || defined(__linux__)
    for(;;)
    {
        CTimeUtils::CPUSleep(2);
    }
    #elif defined(_ESP32)
	vTaskSuspend(NULL); 
    #endif 
}
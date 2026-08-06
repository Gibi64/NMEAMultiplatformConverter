#pragma once
#if defined(_ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#else
#include <thread>
#include <chrono>
#include <sysinfoapi.h>
#endif
class CTimeUtils
{
public:
    struct sDate
    {
        int day;
        int month;
        int year;
    };
    static sDate SystemDate(int NumberOfDaysSince1970)
    {
        sDate s;
        s.day = 0;
        s.month = 0;
        s.year = 0;
        if (NumberOfDaysSince1970 <= 0) return s;
        int MonthDays[] = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
        time_t CumulativeDays = 0;
        time_t CumulativePrevDays = 0;
        int FirstYear = 1970;
        while (CumulativeDays < NumberOfDaysSince1970)
        {
            CumulativePrevDays = CumulativeDays;
            CumulativeDays += 365;
            if (IsBisextil(FirstYear))
            {
                CumulativeDays++;
            }
            FirstYear++;
        }
        s.year = FirstYear - 1;
        int NumberOfDays = (int)(NumberOfDaysSince1970 - CumulativePrevDays);
        bool bBisextil = IsBisextil(s.year);
        if (bBisextil) for (auto i = 1; i < 12; i++) MonthDays[i]++;
        s.month = 0;
        while (MonthDays[s.month] < NumberOfDays) s.month = (s.month + 1) % 12;
        if (s.month > 0) s.day = NumberOfDays - MonthDays[s.month - 1];
        s.month++;
		return s;
    }
    static sDate SystemTime(time_t tsecond)
    {
        sDate s;
        s.day = 0;
        s.month = 0;
        s.year = 0;

        if (!tsecond) return s;
        int MonthDays[] = { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
        time_t t_days = tsecond / 86400;

        //Stockage du nombre de jours à ajouter suivant si l'année est bissextile
        time_t CumulativeDays = 0;
        //Stockage du nombre de jours en fonction du nombre d'année, bissextile ou non
        time_t CumulativePrevDays = 0;
        //année en cours de traitement
        int FirstYear = 1970;


        while (CumulativeDays < t_days)
        {
            CumulativePrevDays = CumulativeDays;
            CumulativeDays += 365;

            if (IsBisextil(FirstYear))
            {
                CumulativeDays++;
            }
        }
        s.year = FirstYear - 1;
        int NumberOfDays = (int)(t_days - CumulativePrevDays);
        bool bBisextil = IsBisextil(s.year);
        if (bBisextil) for (auto i = 1; i < 12; i++) MonthDays[i]++;
        s.month = 0;
        while (MonthDays[s.month] < NumberOfDays) s.month = (s.month + 1) % 12;
        if (s.month > 0) s.day = NumberOfDays - MonthDays[s.month - 1];
        s.month++;

        return s;
    }
    static bool IsBisextil(int year)
    {
        return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
    }
    static unsigned long long GetMs()
    {
        #ifdef _ESP32
        return esp_timer_get_time() / 1000ULL;
        #elif defined(_WIN32)
        return static_cast<uint64_t>(GetTickCount64());
        #elif defined(__linux__)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * 1000ULL + static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;     ²   
        #endif
    }
    static void CPUSleep(int ms)
    {
        #ifdef _ESP32
        vTaskDelay(ms / portTICK_PERIOD_MS);
        #elif defined(_WIN32)
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        #elif defined(__linux__)
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        #endif
    }
};

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
    struct sUTCTime
    {
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int millisecond = 0;

        bool empty() const
        {
            return year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && millisecond == 0;
        }
    };

    static sUTCTime SystemDateTime(uint64_t ms)
    {
            sUTCTime out;

#if defined(_WIN32)
            SYSTEMTIME st;
            GetSystemTime(&st);
            out.year = st.wYear;
            out.month = st.wMonth;
            out.day = st.wDay;
            out.hour = st.wHour;
            out.minute = st.wMinute;
            out.second = st.wSecond;
            out.millisecond = st.wMilliseconds;

#elif defined(__linux__)
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            struct tm tm_utc;
            gmtime_r(&ts.tv_sec, &tm_utc);

            out.year = tm_utc.tm_year + 1900;
            out.month = tm_utc.tm_mon + 1;
            out.day = tm_utc.tm_mday;
            out.hour = tm_utc.tm_hour;
            out.minute = tm_utc.tm_min;
            out.second = tm_utc.tm_sec;
            out.millisecond = ts.tv_nsec / 1000000;

#elif defined(_ESP32)
            // Si tu as un RTC GNSS → utiliser PGN 129029
            // Sinon → temps système (non GNSS)
            struct timeval tv;
            gettimeofday(&tv, nullptr);

            struct tm tm_utc;
            gmtime_r(&tv.tv_sec, &tm_utc);

            out.year = tm_utc.tm_year + 1900;
            out.month = tm_utc.tm_mon + 1;
            out.day = tm_utc.tm_mday;
            out.hour = tm_utc.tm_hour;
            out.minute = tm_utc.tm_min;
            out.second = tm_utc.tm_sec;
            out.millisecond = tv.tv_usec / 1000;

#endif

            return out;
        }
    static bool IsBisextil(int year)
    {
        return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
    }
    static unsigned long long GetMs()
    {
        uint64_t ms;

#ifdef _ESP32
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        ms = tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;

#elif defined(_WIN32)
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        ms = (uli.QuadPart - 116444736000000000ULL) / 10000ULL;

#elif defined(__linux__)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ms = ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
#endif

        // Epoch 2001-01-01 00:00:00 UTC
        static const uint64_t epoch2001 =
            (uint64_t)((uint64_t)978307200ULL * 1000ULL); // seconds * 1000

        return ms - epoch2001;
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

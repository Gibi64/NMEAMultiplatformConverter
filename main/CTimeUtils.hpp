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
    struct sDate
    {
        int day;
        int month;
        int year;
    };
    public:
    class sUTCTime
    {
    public:
        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int millisecond = 0;
        sUTCTime()
        {
            year = 0;
            month = 0;
            day = 0;
            hour = 0;
            minute = 0;
            second = 0;
            millisecond = 0;

        }
        sUTCTime(int y, int mo, int d, int h, int mi, int s, int ms)
        {
            year = y;
            month = mo;
            day = d;
            hour = h;
            minute = mi;
            second = s;
            millisecond = ms;
        }
        bool empty() const
        {
            return year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0 && millisecond == 0;
        }
        int GetIndexValue(int index) const
        {
            switch (index)
            {
            case 0: return year;
            case 1: return month;
            case 2: return day;
            case 3: return hour;
            case 4: return minute;
            case 5: return second;
            case 6: return millisecond;
            }
            return 0;
        }
        void SetIndexValue(int index, int value)
        {
            switch (index)
            {
            case 0: year = value; break;
            case 1: month = value; break;
            case 2: day = value; break;
            case 3: hour = value; break;
            case 4: minute = value; break;
            case 5: second = value; break;
            case 6: millisecond = value; break;
            }
        }
        bool operator<(const sUTCTime& other) const
        {
            for (int i = 0; i < 7; i++)
            {
                int va = GetIndexValue(i);
                int vb = other.GetIndexValue(i);

                if (va < vb) return true;
                if (va > vb) return false;
            }
            return false; // égal
        }
        bool operator>(const sUTCTime& other) const
        {
            for (int i = 0; i < 7; i++)
            {
                int va = GetIndexValue(i);
                int vb = other.GetIndexValue(i);

                if (va > vb) return true;
                if (va < vb) return false;
            }
            return false; // égal
        }
        void IndexPlusPlus(int index)
        {
            // Ajoute 1 à la position de l'index
            sUTCTime Delta = { 0,0,0,0,0,0,0 };
            Delta.SetIndexValue(index, 1);
            (*this)+=Delta;
        }
        sUTCTime& operator+=(const sUTCTime& delta)
        {
            int carry = 0;

            // --- Millisecondes ---
            millisecond += delta.millisecond;
            carry = millisecond / 1000;
            millisecond %= 1000;

            // --- Secondes ---
            second += delta.second + carry;
            carry = second / 60;
            second %= 60;

            // --- Minutes ---
            minute += delta.minute + carry;
            carry = minute / 60;
            minute %= 60;

            // --- Heures ---
            hour += delta.hour + carry;
            carry = hour / 24;
            hour %= 24;

            // --- Jours ---
            day += delta.day + carry;

            // Ajustement des jours selon le mois
            while (true)
            {
                int mdays;

                switch (month)
                {
                case 1: mdays = 31; break;
                case 2: mdays = CTimeUtils::IsBisextil(year) ? 29 : 28; break;
                case 3: mdays = 31; break;
                case 4: mdays = 30; break;
                case 5: mdays = 31; break;
                case 6: mdays = 30; break;
                case 7: mdays = 31; break;
                case 8: mdays = 31; break;
                case 9: mdays = 30; break;
                case 10: mdays = 31; break;
                case 11: mdays = 30; break;
                case 12: mdays = 31; break;
                }

                if (day <= mdays)
                    break;

                day -= mdays;
                month++;
                if (month > 12)
                {
                    month = 1;
                    year++;
                }
            }

            // --- Mois ---
            month += delta.month;
            while (month > 12)
            {
                month -= 12;
                year++;
            }

            // --- Années ---
            year += delta.year;

            return *this;
        }
        virtual unsigned long long ToMs() 
        {
            // On se met en epaoch 01/01/2001
                // Epoch: 01/01/2001
            const int EpochYear = 2001;

            // 1) Convertir les années en jours
            unsigned long long days = 0;
            for (int y = EpochYear; y < year; ++y)
            {
                days += IsBisextil(y) ? 366 : 365;
            }

            // 2) Convertir les mois en jours
            static const int MonthDays[12] =
            { 31,28,31,30,31,30,31,31,30,31,30,31 };

            for (int m = 1; m < month; ++m)
            {
                days += MonthDays[m - 1];
                if (m == 2 && IsBisextil(year)) days++; // février bissextile
            }

            // 3) Ajouter les jours
            days += (day - 1);

            // 4) Convertir en millisecondes
            unsigned long long ms = days * 86400000ULL;
            ms += hour * 3600000ULL;
            ms += minute * 60000ULL;
            ms += second * 1000ULL;
            ms += millisecond;
            return ms;
        }
        sUTCTime operator+(const sUTCTime& delta) const
        {
            sUTCTime tmp = *this;
            tmp += delta;
            return tmp;
        }
    };
    class sDurationTime :sUTCTime
    {
    public:
        sDurationTime()
        {
            sUTCTime();
        }
        sDurationTime(int y, int mo, int d, int h, int mi, int s, int ms)
        {
            year = y;
            month = mo;
            day = d;
            hour = h;
            minute = mi;
            second = s;
            millisecond = ms;
        }
        virtual unsigned long long ToMs() override
        {
            unsigned long long ms;
            ms = millisecond;
            ms += second * 1000UL;
            ms += minute * 60000UL;
            ms += hour * 3600000UL;
            ms += day * 86400000UL;

            ////////////////////////////// TODO : Pour les mois c'est le meme jour du mois suivant ////////
            //          On calcul de dt entre ces deux dates en faisant attention au changement d' année pour le mois suivant 
            //
            if (month)
            {
                auto today = SystemDateTime(GetMs());
                auto nextday = today;
                if (today.month < 12)
                {
                    nextday.month++;
                }
                else
                {
                    nextday.year++;
                }
                ms += (nextday.ToMs() - today.ToMs());
            }
            return ms;
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
    static int GetLastSundayOfMonthInYear(int Month, int Year)
    {
        sUTCTime theDate;
        theDate.year = Year;
        theDate.month = Month;

        // Configuration au 31 Mars de l'année choisie
        int lastDayOfMonth = 31 * (Month == 1 || Month == 3 || Month == 5 || Month == 7 || Month == 8 || Month == 10 || Month == 12)
            + 30 * (Month == 4 || Month == 6 || Month == 9 || Month == 11)
            + 28 * (Month == 2);
        if (IsBisextil(Year) && Month == 2) lastDayOfMonth = 29;
        theDate.day = lastDayOfMonth;
        // Jour du dernier jour du mois
        auto dayLast = GetDayOfWeek(theDate);
        return lastDayOfMonth - dayLast;

    }
    static unsigned long long GetNumberOfDaysSince2001(sUTCTime theDate)
    {
        unsigned long long NbDays = 0;
        for (auto y = 2001; y < theDate.year; y++)
            NbDays += IsBisextil(y) ? 366 : 365;
        for (auto m = 1; m < theDate.month; m++)
        {
            switch (m)
            {
            case 1: case 3: case 5: case 7:case 8:case 10: case 12: NbDays += 31; break;
            case 2:NbDays += IsBisextil(theDate.year) ? 29 : 28; break;
            default : NbDays += 30; break;
            }
        }
        for (auto d = 1; d < theDate.day; d++) NbDays++;
        return NbDays;
    }
    static int GetDayOfWeek(sUTCTime theDate)
    {
        // le 01/01/2001 etait un lundi si on veut avoir 0 pour dimanche on retire 1 au Nombre de jours
        // on ajoute 1 au resultat et on refait un mosulo
        auto DayOfWeek = GetNumberOfDaysSince2001(theDate) % 7 + 1;
        DayOfWeek = DayOfWeek % 7;
        return DayOfWeek;
    }
};

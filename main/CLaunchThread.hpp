#pragma once
#if defined(_WIN32)
#include <winsock2.h>
#include <Windows.h>
#endif

#if defined(_WIN32) || defined(__linux__)
#include <thread>
#elif defined(_ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

class CLaunchThread
{
private:
    bool m_bRunning = false;
#if defined(_WIN32) || defined(__linux__)
    std::thread m_thread;
#elif defined(_ESP32)
    TaskHandle_t m_taskHandle = nullptr;

#endif

public:
    CLaunchThread(void (*function)(void*), void* arg)
    {
        m_bRunning = true;

#if defined(_WIN32) || defined(__linux__)
        m_thread = std::thread([this, function, arg]() {
            function(arg);
            m_bRunning = false;
            });

#elif defined(_ESP32)
xTaskCreatePinnedToCore(
        function,
        "loop",
        4096,
        arg,
        5,
        &m_taskHandle,
        1   // CPU1 = APP_CPU
    );
#endif
    }
    ~CLaunchThread()
    {
#if defined(_WIN32) || defined(__linux__)
        if (m_thread.joinable())
            m_thread.join();
#elif defined(_ESP32)
        if (m_taskHandle)
            vTaskDelete(m_taskHandle);
#endif
    }
};

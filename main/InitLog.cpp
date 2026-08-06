#ifdef _WIN32
#include <stdio.h>
#elif defined __linux__
#include <stdio.h>
#elif defined _ESP32
#include "esp_spiffs.h"
#include "esp_log.h"
#endif
#include <string>
void InitLog()
{
#ifdef _ESP32
esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_vfs_spiffs_register(&conf);
#endif
}
void write_log(const std::string msg)
{
    #ifdef _ESP32
        ESP_LOGI("NMEA","%s\n", msg.c_str());
    #else
        printf("%s\n", msg.c_str());
    #endif  
}
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

#ifndef WIFI_SMARTCONFIG_h
#define WIFI_SMARTCONFIG_h

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data); // WIFI events handler (It manages WIFI credentials and data)
void initialise_wifi(void); // Initiate WIFI mode
void wifi_smartconfig_func(void * parm); // Manage WIFI config with AP
#endif
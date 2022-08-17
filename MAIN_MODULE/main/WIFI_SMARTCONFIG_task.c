#include "string.h"
#include "stdlib.h"
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
#include "esp_now.h"
#include "esp_smartconfig.h"

#include "libraries/WIFI_SMARTCONFIG.h"
#include "config.h"
#include "main.h"

#define MODULE_NAME "WIFI_SMARTCONFIG_TASK"

extern xQueueHandle display_queue;

int wifi_initiated = 0;

char sc_display_msg[32];
char * sc_msg = &sc_display_msg;

extern SemaphoreHandle_t task_semaphore;

//Initialise WIFI SMARTCONFIG 
void wifi_smartconfig_task(void *pvParameters) {

    if(wifi_initiated == 0){
        //Init WIFI SMARCONFIG 
        //Inicializa la particion NVS por defecto.     
        ESP_ERROR_CHECK(nvs_flash_init());

        ESP_LOGI(MODULE_NAME, "Initialise WIFI SMARTCONFIG");
        sprintf(sc_display_msg,"SC Initiating...");
        xQueueSend(display_queue, (void * ) &sc_msg, 10/portTICK_PERIOD_MS);
        delay(200);
    }

    // Task loop
    for (;;) {

        delay(20);
        if(task_semaphore != NULL){

            if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 20000 ) == pdTRUE )) {  
                
                initialise_wifi();
                wifi_initiated++;
                xSemaphoreGive(task_semaphore);

            }
        }else{
            sprintf(sc_display_msg,"SC SEMAFORO OFF");
            xQueueSend(display_queue, (void * ) &sc_msg, 10/portTICK_PERIOD_MS);  
            delay(20);  
        }
        ESP_LOGI(MODULE_NAME, "WIFI initiated");

        // Task delay
        delay(100);
        vTaskDelete(NULL);

    }
    ESP_LOGI(MODULE_NAME, "SMARTCONFIG task finish");
}

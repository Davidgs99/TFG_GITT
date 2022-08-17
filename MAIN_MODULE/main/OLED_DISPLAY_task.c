#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"

#include "config.h"
#include "main.h"
#include "libraries/OLED.h"

#define MODULE_NAME "OLED_DISP_MOD"

extern xQueueHandle display_queue;

char *dis = "Waiting \nmessages...";

uint8_t _color = WHITE;

int oled_initiator = 0;

char buffer_message[127];
char * in_message ;

void display_task(void *pvParameters) {

    ESP_LOGI(MODULE_NAME, "Initiating DISPLAY");

    // INIT Display
    if(oled_initiator == 0){
        initOLED( 128, 64, 21, 22, 16 );
        setFont(ArialMT_Plain_16 );
        clear();
        sendData();

        // Send to screen
        clear();
        drawString(0, 10, "DISPLAY INITIATED", _color );
        sendDataBack();
        delay(200);
    }

    ESP_LOGI(MODULE_NAME, "DISPLAY initiated");
    
    oled_initiator++;

    // Infinite Loop
    for (;;) {

        //Waiting to receive an incoming message.
        if (xQueueReceive(display_queue, (void * )&in_message, (portTickType)portMAX_DELAY)) {

            // Copy message to local buffer
            memcpy(buffer_message, in_message, 127);

            // Send to screen
            clear();
            drawString(0, 10, buffer_message, _color );
            sendDataBack();
            delay(200);

        }
        ESP_LOGI(MODULE_NAME, "OLED DISPLAY task finish");
        delay(20);  
    }
    vTaskDelete(NULL);
}
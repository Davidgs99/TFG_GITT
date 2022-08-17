#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include "esp_http_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "freertos/semphr.h"

#include "libraries/HTTP_SERVER.h"
#include "config.h"
#include "main.h"

#define MODULE_NAME "HTTP_SERVER_TASK"
    
extern xQueueHandle http_server_lora_queue;
extern xQueueHandle http_server_uart_rs485_queue;
extern xQueueHandle display_queue;

char *queue_lora_message;
char *queue_uart_rs485_message;

char server_display_msg[32];
char *server_msg = &server_display_msg;

extern SemaphoreHandle_t task_semaphore;

int server_initiated = 0;

httpd_handle_t server = NULL;               //Create a server which will show sensors measures
    
//Start server and upload messages 
void http_server_task(void *pvParameters){

    // Task loop
    for (;;) {

        // Start web server for the first time
        if (server_initiated == 0){
            if(task_semaphore != NULL){
                if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 10000 ) == pdTRUE )) {
                    delay(20);
                    if(enable_smartconfig == true){
                        server = start_webserver();  
                        ESP_LOGI(MODULE_NAME, "Start web server for the first time\n");
                        sprintf(server_display_msg,"WEB SERVER \nINITIATED");
                        xQueueSend(display_queue, (void * ) &server_msg, 10/portTICK_PERIOD_MS);
                        delay(20);
                        server_initiated++;
                        xSemaphoreGive(task_semaphore);
                    }else{
                        sprintf(server_display_msg,"SERVER WAITING \nWIFI");
                        xQueueSend(display_queue, (void * ) &server_msg, 10/portTICK_PERIOD_MS); 
                        delay(20);
                        server_initiated = 0;
                        xSemaphoreGive(task_semaphore);
                    }
                }
            }
        }
        
        delay(10);
        while(server_initiated > 0){
            
            ESP_LOGI(MODULE_NAME, "SERVER INITIATED\n");
            sprintf(server_display_msg,"WEB SERVER \nON");
            xQueueSend(display_queue, (void * ) &server_msg, 10/portTICK_PERIOD_MS);
            delay(20);

            delay(10);
            if (xQueueReceive(http_server_lora_queue, (void * )&queue_lora_message, (portTickType)portMAX_DELAY)){ //If receive any message from server queue, send it to server
                
                ESP_LOGI(MODULE_NAME, "LORA queue data = %s", (char *)queue_lora_message);
                
                sprintf(&server_string_lora, queue_lora_message);
                
                ESP_LOGI(MODULE_NAME, "Data message received from LORA queue and send to server successfully\n");

            }

            delay(10);
            if (xQueueReceive(http_server_uart_rs485_queue, (void * )&queue_uart_rs485_message, (portTickType)portMAX_DELAY)){    //If receive any message from server queue, send it to server
                
                ESP_LOGI(MODULE_NAME, "MODBUS queue data = %s", (char *)queue_uart_rs485_message);
                
                sprintf(&server_string_modbus, queue_uart_rs485_message);
                
                ESP_LOGI(MODULE_NAME, "Data message received from MODBUS queue and send to server successfully\n");

            }

            break; 

        }

        delay(10000);
        if (server_initiated > 0){
            if(task_semaphore != NULL){
                if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 20000 ) == pdTRUE )) {
                    if(enable_smartconfig == false){
                        stop_webserver(server);
                        ESP_LOGI(MODULE_NAME, "Stop web server\n");
                        sprintf(server_display_msg,"WEB SERVER \nOFF");
                        xQueueSend(display_queue, (void * ) &server_msg, 10/portTICK_PERIOD_MS);
                        delay(20);
                        server_initiated = 0;
                        xSemaphoreGive(task_semaphore);
                    }else{
                        xSemaphoreGive(task_semaphore);
                    }
                }
            }
        }

        // Task delay
        delay(100);
    }

    ESP_LOGI(MODULE_NAME, "HTTP SERVER task finish");
    vTaskDelete(NULL);
}
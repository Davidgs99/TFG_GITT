#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"

#include "libraries/LORA.h"
#include "config.h"
#include "main.h"

#define MODULE_NAME "LORA_RECEIVER_MOD"

extern xQueueHandle http_server_lora_queue;
extern xQueueHandle display_queue;

extern SemaphoreHandle_t task_semaphore;

int lora_initiated = 0;

char lora_display_msg[32];
char *lora_msg = &lora_display_msg;



void writeLoraRegisterMessage( char * message) {

    ESP_LOGI(MODULE_NAME, "Sending message Lora: %s", message);

	loraBeginPacket(false);

	//lora->setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
	loraWrite( (uint8_t*) message, (size_t) strlen(message) );
	loraEndPacket(false);

}

//Receive Lora messages and send them to Lora queue
void lora_receiver_task(void *pvParameters) {

    // NO se inicia hasta 1 segundo despues del arranque
    delay(1000);

    // Inits the Lora module
    ESP_LOGI(MODULE_NAME, "Initiating Lora");
    sprintf(lora_display_msg,"Initiating Lora");
    xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);
    delay(20);

    loraInit( PIN_NUM_MOSI, PIN_NUM_MISO, PIN_NUM_CLK, PIN_NUM_CS, RESET_PIN, PIN_NUM_DIO, 10 );

    // Set the Lora reception mode
    loraReceive(0);
    
    lora_initiated++;
        
    ESP_LOGI(MODULE_NAME, "Lora initiated");
    sprintf(lora_display_msg,"LORA INITIATED");
    xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);
    delay(20);


    //Asign the memory for the incomming message in the heap
    char * in_message = (char *)malloc(LEN_MESSAGES_LORA);
    if(in_message == NULL){
        ESP_LOGE(MODULE_NAME, "%s malloc.1 failed\n", __func__);
    }

    // Task loop
    for (;;) {

        delay(10);
        if(lora_initiated > 0){

            sprintf(lora_display_msg,"LORA waiting \n message...");
            xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);  
            delay(20); 

            if(task_semaphore != NULL){
                if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 20000 ) == pdTRUE )) {
                    
                    enable_lora = true;
                    xSemaphoreGive(task_semaphore);
                    
                }
            }else{
                sprintf(lora_display_msg,"LORA SEMAFORO OFF");
                xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);  
                delay(20);  
            }

            // Check if we have recieved any message
            delay(10);
            if(loraGetDataReceived()){

                BaseType_t lora_status;

                int packetSize = loraHandleDataReceived( in_message );

                loraSetDataReceived( false );

                // SEND THE RECEIVED MESSAGE BY THE QUEUE
                lora_status = xQueueSend(http_server_lora_queue, (void * ) &in_message, 10/portTICK_PERIOD_MS);

                sprintf(lora_display_msg, "Lora Received %s", in_message);
                xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);
                delay(20);

                ESP_LOGI(MODULE_NAME, "LORA received message: %s", (char*)in_message);

                // Check the message has been correctly send into the queue
                if(lora_status == pdPASS){
                    ESP_LOGI(MODULE_NAME, "LORA message send to the server queue correctly");
                }else{
                    ESP_LOGI(MODULE_NAME, "ERROR SENDING MESSAGE");
                }
            }
        }else{
            if(task_semaphore != NULL){
                if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 20000 ) == pdTRUE )) {
                    
                    enable_lora = false;
                    xSemaphoreGive(task_semaphore);
                    
                }
            }else{
                sprintf(lora_display_msg,"LORA SEMAFORO OFF");
                xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);  
                delay(20);  
            }

            sprintf(lora_display_msg,"LORA OFF");
            xQueueSend(display_queue, (void * ) &lora_msg, 10/portTICK_PERIOD_MS);
            delay(20);

        }

        //DELAY AL TERMINAR LA EJECUCIÓN DEL BUCLE
        delay(100);

    }
    ESP_LOGI(MODULE_NAME, "LORA task finish");

    vTaskDelete(NULL);
}
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"
#include "freertos/semphr.h"

#include "libraries/UART_ECHO_RS485.h"
#include "config.h"
#include "main.h"

#define MODULE_NAME "UART_ECHO_RS485_MOD"

extern xQueueHandle http_server_uart_rs485_queue;
extern xQueueHandle display_queue;

extern SemaphoreHandle_t task_semaphore;

const int uart_num = ECHO_UART_PORT;

const uart_config_t uart_config = {
    .baud_rate = 9600,                    
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,     
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,      //otra opcion es UART_HW_FLOWCTRL_RTS
    .rx_flow_ctrl_thresh = 122,
    .source_clk = UART_SCLK_APB,
};

char imp_reg_temperature[8] = {0X01, 0x04, 0x00, 0x01, 0x00, 0x01, 0x60, 0x0A};      //Trama MODBUS para solicitar la lectura discreta del primer registro
char imp_reg_humidity[8] = {0X01, 0x04, 0x00, 0x02, 0x00, 0x01, 0x90, 0x0A};         //Trama MODBUS para solicitar la lectura discreta del registro de la humedad

int uart_initiator = 0;
char *uart_status_display = NULL;
char *uart_data_display = NULL;

char uart_display_msg[32];
char * uart_msg = &uart_display_msg;

//portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

//Receive UART_RS-485 messages and send them to UART_RS-485 queue
void uart_echo_rs485_task(void *pvParameters) {

    // NO se inicia hasta 2 segundos despues del arranque
    delay(2000);

    /*** Inicialización de la UART. ****/
    // SOLO SE EJECUTA LA PRIMERA VEZ

    ESP_LOGI(MODULE_NAME, "Starting UART...");
    
    UARTInit(uart_num , uart_config);

    ESP_LOGI(MODULE_NAME, "Started UART.");
    //uart_get_baudrate(uart_num, *baud);

    sprintf(uart_display_msg,"UART STARTED");
    xQueueSend(display_queue, (void * ) &uart_msg, 10/portTICK_PERIOD_MS);

    //Configuracion del sensor mediante el protocolo general que incluye el dispositivo en su hoja de descripcion
    ESP_LOGI(MODULE_NAME, "1");
    echo_send(uart_num, "TC:2.0", sizeof("TC:2.0"));
    ESP_LOGI(MODULE_NAME, "2");
    echo_send(uart_num, "HC:2.0", sizeof("HC:2.0"));
    ESP_LOGI(MODULE_NAME, "3");
    echo_send(uart_num, "BR:9600", sizeof("BR:9600"));  //PROBAR A PONER EN 116000
    ESP_LOGI(MODULE_NAME, "4");
    echo_send(uart_num, "HZ:2", sizeof("HZ:2"));
 
    // Allocate buffers for UART
    uint8_t* u_data = (uint8_t*) malloc(BUF_SIZE);
    
    /*** BUCLE EJECUCIÓN ****/
    // SIEMPRE UN BUCLE INFINITO EN LAS TAREAS
    for(;;){

        delay(20);

        if(task_semaphore != NULL){

            if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 20000 ) == pdTRUE )) {  
                
                enable_uart = true;
                xSemaphoreGive(task_semaphore);

            }
        }else{
            sprintf(uart_display_msg,"UART SEMAFORO OFF");
            xQueueSend(display_queue, (void * ) &uart_msg, 10/portTICK_PERIOD_MS);
        }

        // Read UART
        int len = uart_read_bytes(uart_num, u_data, (BUF_SIZE - 1), 20/portTICK_PERIOD_MS);
        delay(50);

        //Write u_data to queues
        if (len > 0) {
            
            sprintf(uart_display_msg,"UART\nreceived %d bytes", len);
            xQueueSend(display_queue, (void * ) &uart_msg, 10/portTICK_PERIOD_MS);
            xQueueSend(http_server_uart_rs485_queue, (void * ) &uart_msg, 10/portTICK_PERIOD_MS);
            delay(20);

        } else {

            delay(20);
            
            echo_send(uart_num, (char*) imp_reg_temperature, sizeof(imp_reg_temperature));
            uart_wait_tx_done(uart_num, 10);

            delay(20);

            sprintf(uart_display_msg,"UART send \nrequest");
            xQueueSend(display_queue, (void * ) &uart_msg, 10/portTICK_PERIOD_MS);
            delay(20);
            ESP_LOGI(MODULE_NAME, "UART doesn't received any u_data");

        }

        // Task delay
        delay(100);

    }
    
    //CARLOS LO PUSO AQUI DELAY AL TERMINAR LA EJECUCIÓN DEL BUCLE
    //delay(100);
    vTaskDelete(NULL);

}

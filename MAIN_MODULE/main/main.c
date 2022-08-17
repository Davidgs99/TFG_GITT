// Libraries Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

// Project Includes
#include "main.h"
#include "config.h"
//#include "MODBUS_RECEIVER_task.h"
#include "LORA_RECEIVER_task.h"
#include "WIFI_SMARTCONFIG_task.h"
//#include "libraries/WIFI_STATION.h"
#include "libraries/WIFI_SMARTCONFIG.h"
//#include "libraries/MODBUS_MASTER.h"
#include "libraries/LORA.h"
#include "HTTP_SERVER_task.h"
#include "OLED_DISPLAY_task.h"
#include "libraries/UART_ECHO_RS485.h"
#include "UART_ECHO_RS485_task.h"

#define APP_NAME "TFG_APP"

// FREE RTOS DEFINITIONS
xQueueHandle display_queue = NULL;
xQueueHandle uart_echo_rs485_queue = NULL;
xQueueHandle lora_receiver_queue = NULL;
//xQueueHandle wifi_smartconfig_queue = NULL;
xQueueHandle http_server_lora_queue = NULL;
xQueueHandle http_server_uart_rs485_queue = NULL;

/*
 *  MAIN VARIABLES
 ****************************************************************************************
 */

//Declaration of communications counter
int comm_counter = 0;

extern int uart_initiator;
extern int lora_initiated;

void delay( int msec ) {
    vTaskDelay( msec / portTICK_PERIOD_MS);
}

/*
 *  INIT BOARD FUNCTIONS
 ****************************************************************************************
 */

//Configure GPIO ports
bool getConfigPin() {
    gpio_config_t io_conf;
    gpio_num_t pin = (gpio_num_t) SENDER_RECEIVER_PIN;

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << pin );
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);

    return gpio_get_level( pin ) == 0;
}

/*
*  Declaration of semaphore to manage the access to critical resources 
****************************************************************************************
*  
*/

SemaphoreHandle_t task_semaphore = NULL;

// A task that creates the communications semaphore.
// void vTask( void * pvParameters ) {
// 	// Create the binary semaphore (between 0 & 1) to guard a shared resource.
// 	vSemaphoreCreateBinary( task_semaphore );
// 	ESP_LOGE(APP_NAME, "%s semaphore value\n", (char *)task_semaphore);
// }

/*
 *  MAIN TASK 
 ****************************************************************************************
 * Main task area. 
 */

//Control communications between system functions
void main_task( void* param ){

	// Create the binary semaphore (between 0 & 1) to guard a shared resource.
	vSemaphoreCreateBinary( task_semaphore );
	ESP_LOGE(APP_NAME, "Semaphore value: %d\n", (int)task_semaphore);
		
	// Initialize the default NVS partition. 
	//ESP_ERROR_CHECK(nvs_flash_init());

	// Config Flash Pin
	gpio_num_t m_fp = (gpio_num_t) FLASH_PIN;
	gpio_pad_select_gpio( m_fp );
	gpio_set_direction( m_fp , GPIO_MODE_OUTPUT );

	// Config Flash Pin UART
	gpio_num_t u_m_fp = (gpio_num_t) 21;
	gpio_pad_select_gpio( u_m_fp );
	gpio_set_direction( u_m_fp , GPIO_MODE_OUTPUT );

	// Config Flash Pin Lora
	gpio_num_t l_m_fp = (gpio_num_t) 2;
	gpio_pad_select_gpio( l_m_fp );
	gpio_set_direction( l_m_fp , GPIO_MODE_OUTPUT );

	//Asign the memory for the message in the heap (with the display message size)
	char * message = (char *)malloc(LEN_MESSAGES_SERVER);
	if(message == NULL){
		ESP_LOGE(APP_NAME, "%s malloc.1 failed\n", __func__);
	}

	for ( ;; ) {	

		// Blink the led
		gpio_set_level( m_fp, 1);
		delay(100);

		gpio_set_level( m_fp, 0);
		delay(1000);

		//gpio_set_level( 4, 1);

		if(task_semaphore != NULL){

			if(( xSemaphoreTake( task_semaphore, ( TickType_t ) 10000 ) == pdTRUE )) {

				if(enable_uart == true){
					// Blink the led
					gpio_set_level( u_m_fp, 1);
					delay(100);

					gpio_set_level( u_m_fp, 0);
					delay(1000);
				}

				if(enable_lora == true){
					// Blink the led
					gpio_set_level( l_m_fp, 1);
					delay(100);

					gpio_set_level( l_m_fp, 0);
					delay(1000);
				}

				xSemaphoreGive(task_semaphore);

			}
		}else{
			delay(20);  
		}
	}
    
    ESP_LOGI(APP_NAME, "MAIN task finish");
}

/*
 *  MAIN
 ****************************************************************************************
 * Use to initial configuration of the system and starting the tasks.
 */

//Create queues and init tasks
void app_main() {

	/*** Init the FREERTOS queques ***/

	// Create the queque for sending the LORA messages to HTTP server
    http_server_lora_queue = xQueueCreate(10, sizeof(char *));								
    if( http_server_lora_queue == NULL )
	{
        // There was not enough heap memory space available to create the message buffer. 
        ESP_LOGE(APP_NAME, "Not enough memory to create the http_server_lora_queue\n");

	}

	// Create the queque for sending the MODBUS messages to HTTP server
    http_server_uart_rs485_queue = xQueueCreate(10, sizeof(char *));								
    if( http_server_uart_rs485_queue == NULL )
	{
        // There was not enough heap memory space available to create the message buffer. 
        ESP_LOGE(APP_NAME, "Not enough memory to create the http_server_uart_rs485_queue\n");

	}

	// Create the queque for sending the messages to display
    display_queue = xQueueCreate(30, sizeof(char *));
    if( display_queue == NULL )
	{
        // There was not enough heap memory space available to create the message buffer. 
        ESP_LOGE(APP_NAME, "Not enough memory to create the display_queue\n");

	}


	/*** Init the system tasks ***/

	xTaskCreate(main_task, "main_task", 10000, NULL, 10, NULL);

	xTaskCreate(display_task, "display_task",5000, NULL, 9, NULL);

	xTaskCreate(wifi_smartconfig_task, "wifi_smartconfig_task", 4096, NULL, 8, NULL);

	xTaskCreate(http_server_task, "http_server_task", 10000, NULL, 7, NULL);

	xTaskCreate(uart_echo_rs485_task, "uart_echo_rs485_task", ECHO_TASK_STACK_SIZE, NULL, 6, NULL);

	xTaskCreate(lora_receiver_task, "lora_receiver_task", 10000, NULL, 5, NULL);

}
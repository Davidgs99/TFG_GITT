/*=====================================================================================
 * Description:
 *   The HTTP parameter structures used to create a server which can be able to 
 *   upload information during . Define these structures per your needs in
 *   your application. Below is just an example of possible parameters.
 *====================================================================================*/


/* Uart Events Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <ctype.h>  // toupper y isdigit
#include <math.h>   // pow

#ifndef UART_ECHO_RS485_h
#define UART_ECHO_RS485_h

// // Note: Some pins on target chip cannot be assigned for UART communication.

/* To ESP32 WROOM */
//#define ECHO_UART_PORT   2
// #define ECHO_TEST_TXD   17     
// #define ECHO_TEST_RXD   16    

// // RTS for RS485 Half-Duplex Mode manages DE/~RE
// #define ECHO_TEST_RTS   4 

/* To LORA32 */
#define ECHO_UART_PORT  0
#define ECHO_TEST_TXD   1     
#define ECHO_TEST_RXD   3    

// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define ECHO_TEST_RTS   4

// CTS is not used in RS485 Half-Duplex Mode
#define ECHO_TEST_CTS   (UART_PIN_NO_CHANGE)

#define BUF_SIZE        (128)
#define BAUD_RATE       9600

// Read packet timeout
#define PACKET_READ_TICS        (100 / portTICK_RATE_MS)
#define ECHO_TASK_STACK_SIZE    (2048) //originalmente estaba en 2048
#define ECHO_TASK_PRIO          (6)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged state on receive pin
#define ECHO_READ_TOUT          (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

char slave_address;
int quantity_length;

int caracterHexadecimalADecimal (char caracter);
unsigned long long hexadecimalADecimal (char *cadenaHexadecimal, int longitud);
void read_slave_address (char array_trama[]);
void read_func (char array_trama[]);
void quantity_slave_regs (char array_trama_request[]);
void read_reg_values_from_slave (char array_trama[]);
void read_CRC (char array_trama[]);
void echo_send (const int port, const char* str, uint8_t length);
void UARTInit( const int uart_num , const uart_config_t uart_config );

void delay( int msec );

#endif
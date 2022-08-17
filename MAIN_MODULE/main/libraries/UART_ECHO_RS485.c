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
#include "UART_ECHO_RS485.h"
#include "config.h"

/**
 * This is a example which echos any data it receives on UART back to the sender using RS485 interface in half duplex mode.
*/
#define TAG "RS485_ECHO_APP"

//Convierte los caracteres A, B, C, D, E y F hexadecimales a decimal
int caracterHexadecimalADecimal(char caracter) {
  if (isdigit(caracter))
    return caracter - '0';
  return 10 + (toupper(caracter) - 'A');
}

//Convierte de hexadecimal a decimal desde la potencia mas baja (la ultima posicion del array) hasta la mas alta (la primera posicion del array) 
unsigned long long hexadecimalADecimal(char *cadenaHexadecimal, int longitud) { 
    unsigned long long decimal = 0;
    int potencia = 0;
    for (int i = longitud - 1; i >= 0; i--) {
        // Obtener el decimal, por ejemplo para A es 10, para F 15 y para 9 es 9
        int valorActual = caracterHexadecimalADecimal(cadenaHexadecimal[i]);
        // Elevar 16 a la potencia que se va incrementando, y multiplicarla por el valor
        unsigned long long elevado = pow(16, potencia) * valorActual;
    
        // Agregar al número
        decimal += elevado;
        // Avanzar en la potencia
        potencia++;
    }
    return decimal;
}

void read_slave_address (char array_trama[]){ //lee la direccion a la que se envía la trama
    char slave_add[] = {&array_trama[0]};
    int slave_address = (int)hexadecimalADecimal(slave_add, sizeof(slave_add));
    ESP_LOGI(TAG, "Slave address = %u", slave_address);
}

void read_func (char array_trama[]){ //lee la funcion de la trama
    char slave_func[] = {&array_trama[1]};
    char* function_name;
    int slave_function = (int)hexadecimalADecimal(slave_func, sizeof(slave_func));
    if(slave_function == 3){
        function_name = "Keep read imput register";
    }else if(slave_function == 4){
        function_name = "Read imput register";
    }else{
        function_name = "Unknown function";
    }
    ESP_LOGI(TAG, "Function = %u (%s)", slave_function, function_name);
}

void quantity_slave_regs (char array_trama_request[]){ //lee el numero de registros solicitados (para el sensor de temperatura NUNCA debería ser mayor que 1 con la funcion 04)
    char num_registers[] = {&array_trama_request[4], &array_trama_request[5]};
    quantity_length = (int)hexadecimalADecimal(num_registers, sizeof(num_registers));
    ESP_LOGI(TAG, "\nNumber of registers to read from slave= %u", quantity_length);
}

void read_reg_values_from_slave (char array_trama[]){ //lee el valor Hi y Lo de cada registro
    int contador = 0;
    int value[quantity_length]; //almacena el valor decial de cada registro leido
    char reg_value[2];
    for (int i = 3; i < 3 + (quantity_length * 2); i++, i++){
        ESP_LOGI(TAG, "\nEmpieza el registro en = %u", i);
        reg_value[0] = &array_trama[i];
        reg_value[1] = &array_trama[i+1];
        value[contador] = (int)hexadecimalADecimal(reg_value, sizeof(reg_value));
        contador++;
        ESP_LOGI(TAG, "\nValue of register %u = %u", contador, value[contador]);
    }   
}

void read_CRC (char array_trama[]){
    ESP_LOGI(TAG, "\nCRC = %s %s", (char*)&array_trama[6], (char*)&array_trama[7]);
    
}

void echo_send(const int port, const char* str, uint8_t length){

    if (uart_write_bytes(port, str, length) != length) {
        ESP_LOGE(TAG, "Send data critical failure.");
        abort();
    }
}

void UARTInit( const int uart_num , const uart_config_t uart_config ) {
	
    ESP_LOGI(TAG, "Install and configure UART.");

    // Set UART log level
    //esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Start RS485 application test and configure UART.");

    // Install UART driver (we don't need an event queue here)
    // In this example we don't even use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(uart_num, 1024 * 2, 1024 * 2, 0, NULL, 0));

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config)); //quitar si da problemas y ajustar el baud rate del sensor a 115200 a ver si funciona 0:01 02/06/22

    ESP_LOGI(TAG, "UART set pins, mode and install driver.");

    // Set UART pins as per KConfig settings
    ESP_ERROR_CHECK(uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Set RS485 half duplex mode
    ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_UART));

    // Set read timeout of UART TOUT feature
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, ECHO_READ_TOUT));  

    //uart_get_baudrate(uart_num, baud);

}
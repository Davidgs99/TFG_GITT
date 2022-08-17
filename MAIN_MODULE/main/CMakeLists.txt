idf_component_register(SRCS "main.c" "UART_ECHO_RS485_task.c" "LORA_RECEIVER_task.c" "OLED_DISPLAY_task.c" "WIFI_SMARTCONFIG_task.c" "HTTP_SERVER_task.c" 
                            "libraries/UART_ECHO_RS485.c" "libraries/LORA.c" "libraries/OLED.c" "libraries/font.c" "libraries/WIFI_SMARTCONFIG.c"
                            "libraries/HTTP_SERVER.c" 
                    INCLUDE_DIRS ".")
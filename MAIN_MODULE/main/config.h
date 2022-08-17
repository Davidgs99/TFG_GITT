#ifndef CONFIG
#define CONFIG

/*** Board Configuration ***/
#define PIN_NUM_MISO 	19
#define PIN_NUM_MOSI 	27
#define PIN_NUM_CLK  	5
#define PIN_NUM_CS   	18
#define PIN_NUM_DIO		26
#define RESET_PIN  		23

#define SENDER_RECEIVER_PIN	12
#define	FLASH_PIN			25


/*** OLED DISPLAY System config ***/
#define LEN_MESSAGES_OLED 32 


/*** MODBUS protocol System config ***/
#define LEN_MESSAGES_UART 32              
uint32_t *baud;

//Control of modbus inicialization
int mb_init_crt;
char buffer_TXD[128];       //Create a buffer for TXD 
char buffer_RXD[128];       //Create a buffer for RXD 


/*** LORA protocol System config ***/
#define LEN_MESSAGES_LORA 32                



/*** WIFI protocol System config ***/
#define LEN_MESSAGES_WIFI 64              //CHECK IT  

bool enable_wifi;
bool enable_smartconfig;
bool enable_uart;
bool enable_lora;
void *smartconfig_param;

/*ROUTER LAB*/
#define ESP_WIFI_SSID      "Sitecom5982F4\n"
#define ESP_WIFI_PASS      "password\n"

/*ROUTER CASA*/
// #define ESP_WIFI_SSID      "MOBISTAR_3BB0\n"
// #define ESP_WIFI_PASS      "vBKLAMpLafwQhrN8JEew\n"
#define ESP_MAXIMUM_RETRY  3

char *wf_ssid;
char *wf_password;

/*** HTTP Server System config ***/
#define LEN_MESSAGES_SERVER 80              //CHECK IT  

const char server_string_lora[LEN_MESSAGES_SERVER];
const char server_string_modbus[LEN_MESSAGES_SERVER];

#endif
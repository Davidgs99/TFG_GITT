/*=====================================================================================
 * Description:
 *   The HTTP parameter structures used to create a server which can be able to 
 *   upload information during . Define these structures per your needs in
 *   your application. Below is just an example of possible parameters.
 *====================================================================================*/


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

#ifndef HTTP_SERVER_h
#define HTTP_SERVER_h

#define USERNAME    "user"              //HTTP server user
#define PASSWORD    "password"          //HTTP server password

char *http_auth_basic(const char *username, const char *password);
esp_err_t basic_auth_get_handler(httpd_req_t *req);                 /* An HTTP GET handler */
void httpd_register_basic_auth(httpd_handle_t server);
esp_err_t data_get_handler(httpd_req_t *req);
esp_err_t echo_post_handler(httpd_req_t *req);
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);
esp_err_t ctrl_put_handler(httpd_req_t *req);
httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);
void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif


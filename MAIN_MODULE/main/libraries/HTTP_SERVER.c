/* Simple HTTP Server Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "HTTP_SERVER.h"
#include "config.h"

const char *HTTP_SERVER_TAG = "HTTP SERVER LIBRARY";

typedef struct {            //Structure which contains username and password
    char *username;
    char *password;
} basic_auth_info_t;

#define HTTPD_401   "401 UNAUTHORIZED"           /*!< HTTP Response 401 */

char *http_auth_basic(const char *username, const char *password) {
    int out;
    char *user_info = NULL;
    char *digest = NULL;
    size_t n = 0;
    asprintf(&user_info, "%s:%s", username, password);
    if (!user_info) {
        ESP_LOGE(HTTP_SERVER_TAG, "No enough memory for user information");
        return NULL;
    }
    esp_crypto_base64_encode(NULL, 0, &n, (const unsigned char *)user_info, strlen(user_info));

    /* 6: The length of the "Basic " string
    * n: Number of bytes for a base64 encode format
    * 1: Number of bytes for a reserved which be used to fill zero
    */
    digest = calloc(1, 6 + n + 1);
    if (digest) {
        strcpy(digest, "Basic ");
        esp_crypto_base64_encode((unsigned char *)digest + 6, n, (size_t *)&out, (const unsigned char *)user_info, strlen(user_info));
    }
    free(user_info);
    return digest;
}

//Handle basic authentication data
esp_err_t basic_auth_get_handler(httpd_req_t *req) {
    char *buf = NULL;
    size_t buf_len = 0;
    basic_auth_info_t *basic_auth_info = req->user_ctx;

    buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
    if (buf_len > 1) {
        buf = calloc(1, buf_len);
        if (!buf) {
            ESP_LOGE(HTTP_SERVER_TAG, "No enough memory for basic authorization");
            return ESP_ERR_NO_MEM;
        }

        if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Authorization: %s", buf);
        } else {
            ESP_LOGE(HTTP_SERVER_TAG, "No auth value received");
        }

        char *auth_credentials = http_auth_basic(basic_auth_info->username, basic_auth_info->password);
        if (!auth_credentials) {
            ESP_LOGE(HTTP_SERVER_TAG, "No enough memory for basic authorization credentials");
            free(buf);
            return ESP_ERR_NO_MEM;
        }

        if (strncmp(auth_credentials, buf, buf_len)) {
            ESP_LOGE(HTTP_SERVER_TAG, "Not authenticated");
            httpd_resp_set_status(req, HTTPD_401);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            //httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"module_data\"");

            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"lora_data\"");

            httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"modbus_data\"");

            httpd_resp_send(req, NULL, 0);
        } else {
            ESP_LOGI(HTTP_SERVER_TAG, "Authenticated!");
            char *basic_auth_resp = NULL;
            httpd_resp_set_status(req, HTTPD_200);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Connection", "keep-alive");
            asprintf(&basic_auth_resp, "{\"authenticated\": true,\"user\": \"%s\"}", basic_auth_info->username);
            if (!basic_auth_resp) {
                ESP_LOGE(HTTP_SERVER_TAG, "No enough memory for basic authorization response");
                free(auth_credentials);
                free(buf);
                return ESP_ERR_NO_MEM;
            }
            httpd_resp_send(req, basic_auth_resp, strlen(basic_auth_resp));
            free(basic_auth_resp);
        }
        free(auth_credentials);
        free(buf);
    } else {
        ESP_LOGE(HTTP_SERVER_TAG, "No auth header received");
        httpd_resp_set_status(req, HTTPD_401);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Connection", "keep-alive");
        //httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"module_data\"");

        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"lora_data\"");

        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"modbus_data\"");

        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

//Handler to operate with basic authentication data
httpd_uri_t basic_auth = {
    .uri       = "/basic_auth",
    .method    = HTTP_GET,
    .handler   = basic_auth_get_handler,
};

//Handler to operate with basic authentication data
void httpd_register_basic_auth(httpd_handle_t server) {

    basic_auth_info_t *basic_auth_info = calloc(1, sizeof(basic_auth_info_t));
    if (basic_auth_info) {
        basic_auth_info->username = USERNAME;
        basic_auth_info->password = PASSWORD;

        basic_auth.user_ctx = basic_auth_info;
        httpd_register_uri_handler(server, &basic_auth);
    }
}

/* Upload data into the server*/                                   
esp_err_t data_get_handler(httpd_req_t *req) {
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(HTTP_SERVER_TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTP_SERVER_TAG, "Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTP_SERVER_TAG, "Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(HTTP_SERVER_TAG, "Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(HTTP_SERVER_TAG, "Request headers lost");
    }
    return ESP_OK;
}
 
const httpd_uri_t lora_data = {                                 
    .uri       = "/lora_data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    /* Let's pass response string in user context to demonstrate it's usage */
    .user_ctx  = &server_string_lora,
};

const httpd_uri_t modbus_data = {                                 
    .uri       = "/modbus_data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    /* Let's pass response string in user context to demonstrate it's usage */
    .user_ctx  = &server_string_modbus,
};
 
/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /lora_data and /modbus_data URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /lora_data or /modbus_data is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /modbus_data)
 * or keeps it open (when requested URI is /lora_data). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 * 
 * REVISAR
 * 
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if (strcmp("/lora_data", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/lora_data URI is not available");
        // Return ESP_OK to keep underlying socket open
        return ESP_OK;

    } else if (strcmp("/modbus_data", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/modbus_data URI is not available");
        // Return ESP_FAIL to close underlying socket
        return ESP_FAIL;
        
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
esp_err_t ctrl_put_handler(httpd_req_t *req) {
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(HTTP_SERVER_TAG, "Unregistering /lora_data and /modbus_data URIs");
        httpd_unregister_uri(req->handle, "/lora_data");
        httpd_unregister_uri(req->handle, "/modbus_data");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else {
        ESP_LOGI(HTTP_SERVER_TAG, "Registering /lora_data URI");
        httpd_register_uri_handler(req->handle, &lora_data);
        ESP_LOGI(HTTP_SERVER_TAG, "Registering /modbus_data URI");
        httpd_register_uri_handler(req->handle, &modbus_data);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

const httpd_uri_t ctrl = {                                                      //Disconnect handlers (lora_data / modbus_data)
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

//Start webserver
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;                       //Define a handler of this server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();     //Set HTTP config with a default config macro
    config.lru_purge_enable = true;                     //Enable Least Recently Used connection purge. Let use created communication sesion after lose WIFI connection 

    // Start the httpd server
    ESP_LOGI(HTTP_SERVER_TAG, "Starting server on port: '%d'", config.server_port); //Use port 80 to start server
    if (httpd_start(&server, &config) == ESP_OK) {                                  //Check if server's initiation was successful
        // Set URI handlers                                         
        ESP_LOGI(HTTP_SERVER_TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &lora_data);
        httpd_register_uri_handler(server, &modbus_data);
        httpd_register_uri_handler(server, &ctrl);
        httpd_register_basic_auth(server);
        ESP_LOGI(HTTP_SERVER_TAG, "URI handlers registered");
        return server;
    }

    ESP_LOGI(HTTP_SERVER_TAG, "Error starting server!");
    return NULL;
}

/*Stop the HTTP server*/
void stop_webserver(httpd_handle_t server){
    httpd_stop(server);
}

//Diconnect server
void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(HTTP_SERVER_TAG, "Stopping TFG_DGS webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

//Check server value to initialize it when it is NULL
void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    httpd_handle_t* server = (httpd_handle_t*) arg;     
    if (*server == NULL) {
        ESP_LOGI(HTTP_SERVER_TAG, "Starting TFG_DGS webserver");
        *server = start_webserver();                                                                //Start webserver with default configuration
    }
}

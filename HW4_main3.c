#include <string.h> 
#include <sys/param.h> 
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h" 
#include "esp_system.h" 
#include "esp_wifi.h" 
#include "esp_event.h" 
#include "esp_log.h" 
#include "nvs_flash.h" 
#include "esp_http_server.h" 
#include "driver/gpio.h" 
 
#define EXAMPLE_ESP_WIFI_SSID      "ESP32" 
#define EXAMPLE_ESP_WIFI_PASS      "12345678"  
#define EXAMPLE_MAX_STA_CONN       4 
 
static const char *TAG = "wifi_softAP"; 
 
void wifi_init_softap() { 
    ESP_ERROR_CHECK(esp_netif_init()); 
    ESP_ERROR_CHECK(esp_event_loop_create_default()); 
    esp_netif_create_default_wifi_ap(); 
 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); 
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); 
 
    wifi_config_t wifi_config = { 
        .ap = { 
            .ssid = EXAMPLE_ESP_WIFI_SSID, 
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID), 
            .password = EXAMPLE_ESP_WIFI_PASS, 
            .max_connection = EXAMPLE_MAX_STA_CONN, 
            .authmode = WIFI_AUTH_WPA_WPA2_PSK 
        }, 
    }; 
 
    if (strlen(EXAMPLE_ESP_WIFI_PASS) < 8) { 
        // Set to open network if password is less than 8 characters 
        wifi_config.ap.authmode = WIFI_AUTH_OPEN; 
    } 
 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); 
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)); 
    ESP_ERROR_CHECK(esp_wifi_start()); 
 
    ESP_LOGI(TAG, "Wi-Fi AP set with SSID: %s", EXAMPLE_ESP_WIFI_SSID); 
} 
 
 
/* Function to handle HTTP GET requests */ 
esp_err_t get_handler(httpd_req_t *req) { 
char* resp_str = (char*) "ESP32 Web Server: Send POST request to control LED"; 
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN); 
    return ESP_OK; 
} 
 
/* Function to handle HTTP POST requests */ 
esp_err_t echo_post_handler(httpd_req_t *req) 
{ 
    char buf[100]; 
    int ret, remaining = req->content_len; 
 
    while (remaining > 0) { 
        /* Read the data for the request */ 
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) { 
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) { 
                /* Retry receiving if timeout occurred */ 
                continue; 
            } 
            return ESP_FAIL; 
        } 
        buf[ret] = '\0'; 
 
        // Check for LED command 
        if (strcmp(buf,"{\"command\":\"on\"}") == 0) { 
            gpio_set_level(GPIO_NUM_2, 1); // Turn LED on 
            httpd_resp_send(req, "LED is turned on", HTTPD_RESP_USE_STRLEN); 
        }  
        else if (strcmp(buf, "{\"command\":\"off\"}") == 0) { 
            gpio_set_level(GPIO_NUM_2, 0); // Turn LED off 
            httpd_resp_send(req, "LED is turned off", HTTPD_RESP_USE_STRLEN); 
        }  
        else { 
            httpd_resp_send(req, "Command not recognized", HTTPD_RESP_USE_STRLEN); 
        } 
        /* Send back the same data */ 
        httpd_resp_send_chunk(req, buf, ret); 
        remaining -= ret; 
 
        /* Log data received */ 
        ESP_LOGI(TAG, "=========== RECEIVED DATA =========="); 
        ESP_LOGI(TAG, "%.*s", ret, buf); 
        ESP_LOGI(TAG, "===================================="); 
    } 
 
    // End response 
    httpd_resp_send_chunk(req, NULL, 0); 
    return ESP_OK; 
} 
/* Start the web server */ 
httpd_handle_t start_webserver(void) { 
httpd_config_t config = HTTPD_DEFAULT_CONFIG(); 
 
    // Start the httpd server 
    httpd_handle_t server = NULL; 
    if (httpd_start(&server, &config) == ESP_OK) { 
        // Set URI handlers 
        httpd_uri_t get_uri = { 
            .uri       = "/response", 
            .method    = HTTP_GET, 
            .handler   = get_handler, 
            .user_ctx  = NULL 
        }; 
        httpd_register_uri_handler(server, &get_uri); 
 
        httpd_uri_t post_uri = { 
            .uri       = "/request", 
            .method    = HTTP_POST, 
            .handler   = echo_post_handler, 
            .user_ctx  = NULL 
        }; 
        httpd_register_uri_handler(server, &post_uri); 
    } 
    return server; 
} 
 
void

app_main() { 
    // Initialize NVS 
    esp_err_t ret = nvs_flash_init(); 
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { 
      ESP_ERROR_CHECK(nvs_flash_erase()); 
      ret = nvs_flash_init(); 
    } 
    ESP_ERROR_CHECK(ret); 
 
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP"); 
    wifi_init_softap(); 
     
    gpio_pad_select_gpio(GPIO_NUM_2); 
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT); 
    // Start the HTTP Server 
    start_webserver(); 
}
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "global_state.h"
#include "asic.h"

// static const char *TAG = "asic_api";
static GlobalState *GLOBAL_STATE = NULL;

// Function declarations from http_server.c
extern esp_err_t is_network_allowed(httpd_req_t *req);
extern esp_err_t set_cors_headers(httpd_req_t *req);

// Initialize the ASIC API with the global state
void asic_api_init(GlobalState *global_state) {
    GLOBAL_STATE = global_state;
}

/* Handler for system asic endpoint */
esp_err_t GET_system_asic(httpd_req_t *req)
{
    if (is_network_allowed(req) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    }

    httpd_resp_set_type(req, "application/json");

    // Set CORS headers
    if (set_cors_headers(req) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    cJSON *root = cJSON_CreateObject();

    // Add ASIC model to the JSON object
    cJSON_AddStringToObject(root, "ASICModel", GLOBAL_STATE->DEVICE_CONFIG.family.asic.name);
    
    cJSON_AddNumberToObject(root, "defaultFrequency", GLOBAL_STATE->DEVICE_CONFIG.family.asic.default_frequency_mhz);

    // Create arrays for frequency and voltage options based on ASIC model
    cJSON *freqOptions = cJSON_CreateArray();
    size_t count = 0;
    while (GLOBAL_STATE->DEVICE_CONFIG.family.asic.frequency_options[count] != 0) {
        cJSON_AddItemToArray(freqOptions, cJSON_CreateNumber(GLOBAL_STATE->DEVICE_CONFIG.family.asic.frequency_options[count]));
        count++;
    }
    cJSON_AddItemToObject(root, "frequencyOptions", freqOptions);

    cJSON_AddNumberToObject(root, "defaultVoltage", GLOBAL_STATE->DEVICE_CONFIG.family.asic.default_voltage_mv);

    cJSON *voltageOptions = cJSON_CreateArray();
    count = 0;
    while (GLOBAL_STATE->DEVICE_CONFIG.family.asic.voltage_options[count] != 0) {
        cJSON_AddItemToArray(voltageOptions, cJSON_CreateNumber(GLOBAL_STATE->DEVICE_CONFIG.family.asic.voltage_options[count]));
        count++;
    }
    cJSON_AddItemToObject(root, "voltageOptions", voltageOptions);

    const char *response = cJSON_Print(root);
    httpd_resp_sendstr(req, response);

    free((void *)response);
    cJSON_Delete(root);
    return ESP_OK;
}

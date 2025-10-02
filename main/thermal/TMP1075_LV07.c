// TODO: https://github.com/espressif/esp-idf/commit/89ba620cfd848bf6c90cac37114dbb99651b0120

#include <stdio.h>
#include "esp_log.h"
#include "TMP1075_LV07.h"

/**
 * @brief Initialize the TMP1075 sensor.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t TMP1075_LV07_init(tmp1075_t *sensor, uint8_t i2c_address, const char *TAG) {
    if (sensor == NULL || TAG == NULL) {
        ESP_LOGE("TMP1075_LV07", "NULL pointer in sensor or tag");
        return ESP_FAIL;
    }

    sensor->TAG = TAG;

    ESP_LOGI(sensor->TAG, "Initializing TMP1075_LV07 at 0x%02X", i2c_address);

    if (i2c_bitaxe_add_device(i2c_address, &sensor->dev_handle, sensor->TAG) != ESP_OK) {
        ESP_LOGE(sensor->TAG, "Failed to add device");
        return ESP_FAIL;
    }

    return ESP_OK;
}

float TMP1075_LV07_read_temperature(tmp1075_t *sensor) {
    uint8_t data[2] = {0};
    esp_err_t err;

    if (sensor == NULL) {
        ESP_LOGE("TMP1075_LV07", "NULL pointer in sensor");
        return ESP_FAIL;
    }

    err = i2c_bitaxe_register_read(sensor->dev_handle, TMP1075_LV07_TEMP_REG, data, 2);
    if (err != ESP_OK) {
        ESP_LOGE(sensor->TAG, "Failed to read temperature: %s", esp_err_to_name(err));
        return 0;
    }

    // Combine MSB + upper nibble of LSB into a signed 12‐bit value:
    //      raw12 = (data[0] << 4) | (data[1] >> 4)
    int16_t raw12 = ((int16_t)data[0] << 4) | ((int16_t)(data[1] >> 4) & 0x0F);

    // If bit 11 is set, that means a negative temperature—sign‐extend:
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;  // set bits 15..12 = 1
    }

    // Each LSB corresponds to 0.0625 °C. Multiply raw12 by 0.0625f to get a float °C:
    float temp = raw12 * 0.0625f;
    return temp;
}

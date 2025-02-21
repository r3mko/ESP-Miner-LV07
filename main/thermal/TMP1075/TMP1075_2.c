// TODO: https://github.com/espressif/esp-idf/commit/89ba620cfd848bf6c90cac37114dbb99651b0120

#include <stdio.h>
#include "esp_log.h"
#include "i2c_bitaxe.h"

#include "TMP1075_2.h"

static const char *TAG = "TMP1075_2";

static i2c_master_dev_handle_t tmp1075_2_dev_handle;

/**
 * @brief Initialize the TMP1075 sensor.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t TMP1075_2_init(void) {
    ESP_LOGI(TAG, "Initializing: Temperature sensor 2");

    return i2c_bitaxe_add_device(TMP1075_2_I2CADDR_DEFAULT, &tmp1075_2_dev_handle, TAG);
}

uint8_t TMP1075_2_read_temperature(void) {
    uint8_t data[2];

    ESP_ERROR_CHECK(i2c_bitaxe_register_read(tmp1075_2_dev_handle, TMP1075_2_TEMP_REG, data, 2));
    return data[0];
}

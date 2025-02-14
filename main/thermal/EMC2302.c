#include <stdio.h>
#include "esp_log.h"

#include "i2c_bitaxe.h"
#include "EMC2302.h"

static const char *TAG = "EMC2302";

static i2c_master_dev_handle_t emc2302_dev_handle;

esp_err_t EMC2302_init(bool invertPolarity) {
    if (i2c_bitaxe_add_device(EMC2302_I2CADDR_DEFAULT, &emc2302_dev_handle, TAG) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "EMC2302 init with polarity %d", invertPolarity);

    // Set fan range to 00: 500 RPM minimum, TACH count multiplier = 1
    // Fan config (default) before register write: 2B = 00101011
    ESP_ERROR_CHECK(i2c_bitaxe_register_write_byte(emc2302_dev_handle, EMC2302_FAN1_CONFIG1, 0b00001011));
    ESP_ERROR_CHECK(i2c_bitaxe_register_write_byte(emc2302_dev_handle, EMC2302_FAN2_CONFIG1, 0b00001011));

    if (invertPolarity) {
        ESP_ERROR_CHECK(i2c_bitaxe_register_write_byte(emc2302_dev_handle, EMC2302_PWM_POLARITY, 0b00000011));
    }

    return ESP_OK;
}

// Set fan speed to a given percent
void EMC2302_set_fan_speed(uint8_t devicenum, float percent)
{
    uint8_t speed;
	uint8_t FAN_SETTING_REG = EMC2302_FAN1_SETTING + (devicenum * 0x10);

    speed = (uint8_t) (255.0 * (1.0f - percent));
    ESP_ERROR_CHECK(i2c_bitaxe_register_write_byte(emc2302_dev_handle, FAN_SETTING_REG, speed));
}

// Get fan speed
uint16_t EMC2302_get_fan_speed(uint8_t devicenum)
{
    uint8_t tach_lsb, tach_msb;
    uint16_t RPM;
    uint8_t TACH_LSB_REG = EMC2302_TACH1_LSB + (devicenum * 0x10);
    uint8_t TACH_MSB_REG = EMC2302_TACH1_MSB + (devicenum * 0x10);

    ESP_ERROR_CHECK(i2c_bitaxe_register_read(emc2302_dev_handle, TACH_LSB_REG, &tach_lsb, 1));
    ESP_ERROR_CHECK(i2c_bitaxe_register_read(emc2302_dev_handle, TACH_MSB_REG, &tach_msb, 1));

    RPM = (tach_msb << 5) + ((tach_lsb >> 3) & 0x1F);
    RPM = EMC2302_FAN_RPM_NUMERATOR / RPM;

    // DEBUG: Get fan speed and config
    //
    //ESP_LOGI(TAG, "Raw Fan Speed[%d] = %02X %02X", devicenum, tach_msb, tach_lsb);
    //ESP_LOGI(TAG, "Fan Speed[%d] = %d RPM", devicenum, RPM);
    //
    //uint8_t fan_conf;
    //uint8_t FAN_CONFIG1 = EMC2302_FAN1_CONFIG1 + (devicenum * 0x10);
    //
    //ESP_ERROR_CHECK(i2c_bitaxe_register_read(emc2302_dev_handle, FAN_CONFIG1, &fan_conf, 1));
    //ESP_LOGI(TAG, "Fan config[%d] = %02X", devicenum, fan_conf);

    return RPM;
}

float EMC2302_get_external_temp(void)
{
    // We don't have temperature on this chip, so fake it
    return 0;
}

uint8_t EMC2302_get_internal_temp(void)
{
    // We don't have temperature on this chip, so fake it
    return 0;
}
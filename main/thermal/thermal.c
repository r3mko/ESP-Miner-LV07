#include "thermal.h"

#include "esp_log.h"
#include "EMC2101.h"
#include "EMC2103.h"
#include "EMC2302_LV07.h"
#include "TMP1075_LV07.h"

static const char * TAG = "thermal";

tmp1075_t sensor_A, sensor_B;

esp_err_t Thermal_init(DeviceConfig * DEVICE_CONFIG)
{
    if (DEVICE_CONFIG->EMC2101) {
        esp_err_t res = EMC2101_init(DEVICE_CONFIG->emc_temp_offset);
        // TODO: Improve this check.
        if (DEVICE_CONFIG->emc_ideality_factor != 0x00) {
            ESP_LOGI(TAG, "EMC2101 configuration: Ideality Factor: %02x, Beta Compensation: %02x", DEVICE_CONFIG->emc_ideality_factor, DEVICE_CONFIG->emc_beta_compensation);
            EMC2101_set_ideality_factor(DEVICE_CONFIG->emc_ideality_factor);
            EMC2101_set_beta_compensation(DEVICE_CONFIG->emc_beta_compensation);
        }
        return res;
    }
    if (DEVICE_CONFIG->EMC2103) {
        return EMC2103_init(DEVICE_CONFIG->emc_temp_offset);
    }
    if (DEVICE_CONFIG->EMC2302_LV07) {
        ESP_LOGI(TAG, "Initializing EMC2302_LV07 and TMP1075_LV07s (Temperature offset: %d °C)", DEVICE_CONFIG->emc_temp_offset);
        esp_err_t res_emc2302_LV07 = EMC2302_LV07_init();
        esp_err_t res_tmp1075_A    = TMP1075_LV07_init(&sensor_A, DEVICE_CONFIG->TMP1075_A, "TMP1075_A", DEVICE_CONFIG->emc_temp_offset);
        esp_err_t res_tmp1075_B    = TMP1075_LV07_init(&sensor_B, DEVICE_CONFIG->TMP1075_B, "TMP1075_B", DEVICE_CONFIG->emc_temp_offset);

        // return the first non-ESP_OK, or ESP_OK if all succeed
        if (res_emc2302_LV07 != ESP_OK) return res_emc2302_LV07;
        if (res_tmp1075_A    != ESP_OK) return res_tmp1075_A;
        if (res_tmp1075_B    != ESP_OK) return res_tmp1075_B;

        return ESP_OK;
    }

    return ESP_FAIL;
}

//percent is a float between 0.0 and 1.0
esp_err_t Thermal_set_fan_percent(DeviceConfig * DEVICE_CONFIG, float percent)
{
    if (DEVICE_CONFIG->EMC2101) {
        return EMC2101_set_fan_speed(percent);
    }
    if (DEVICE_CONFIG->EMC2103) {
        return EMC2103_set_fan_speed(percent);
    }
    if (DEVICE_CONFIG->EMC2302_LV07) {
        return EMC2302_LV07_set_fan_speed(0, percent);
        return EMC2302_LV07_set_fan_speed(1, percent);
    }

    return ESP_OK;
}

uint16_t Thermal_get_fan_speed(DeviceConfig * DEVICE_CONFIG) 
{
    if (DEVICE_CONFIG->EMC2101) {
        return EMC2101_get_fan_speed();
    }
    if (DEVICE_CONFIG->EMC2103) {
        return EMC2103_get_fan_speed();
    }
    if (DEVICE_CONFIG->EMC2302_LV07) {
        return EMC2302_LV07_get_fan_speed(0);
    }

    return 0;
}

float Thermal_get_chip_temp(GlobalState * GLOBAL_STATE)
{
    if (!GLOBAL_STATE->ASIC_initalized) {
        return -1;
    }

    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2101) {
        if (GLOBAL_STATE->DEVICE_CONFIG.emc_internal_temp) {
            return EMC2101_get_internal_temp();
        } else {
            return EMC2101_get_external_temp();
        }
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2103) {
        return EMC2103_get_external_temp();
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2302_LV07) {
        return TMP1075_LV07_read_temperature(&sensor_A);
    }
    return -1;
}

float Thermal_get_chip_temp2(GlobalState * GLOBAL_STATE)
{
    if (!GLOBAL_STATE->ASIC_initalized) {
        return -1;
    }

    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2103) {
        return EMC2103_get_external_temp2();
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2302_LV07) {
        return TMP1075_LV07_read_temperature(&sensor_B);
    }
    return -1;
}

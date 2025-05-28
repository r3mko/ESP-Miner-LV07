#include "thermal.h"

esp_err_t Thermal_init(DeviceConfig device_config)
{
    if (device_config.EMC2101) {
        esp_err_t res = EMC2101_init();
        // TODO: Improve this check.
        if (device_config.emc_ideality_factor != 0x00) {
            EMC2101_set_ideality_factor(device_config.emc_ideality_factor);
            EMC2101_set_beta_compensation(device_config.emc_beta_compensation);
        }
        return res;
    }
    if (device_config.EMC2103) {
        return EMC2103_init();
    }
    if (device_config.EMC2302) {
        esp_err_t res_emc2302   = EMC2302_init();
        esp_err_t res_tmp1075_1 = TMP1075_1_init();
        esp_err_t res_tmp1075_2 = TMP1075_2_init();

        // return the first non-ESP_OK, or ESP_OK if all succeed
        if (res_emc2302   != ESP_OK) return res_emc2302;
        if (res_tmp1075_1 != ESP_OK) return res_tmp1075_1;
        if (res_tmp1075_2 != ESP_OK) return res_tmp1075_2;

        return ESP_OK;
    }

    return ESP_FAIL;
}

//percent is a float between 0.0 and 1.0
esp_err_t Thermal_set_fan_percent(DeviceConfig device_config, float percent)
{
    if (device_config.EMC2101) {
        EMC2101_set_fan_speed(percent);
    }
    if (device_config.EMC2103) {
        EMC2103_set_fan_speed(percent);
    }
    if (device_config.EMC2302) {
        EMC2302_set_fan_speed(0, percent);
        EMC2302_set_fan_speed(1, percent);
    }

    return ESP_OK;
}

uint16_t Thermal_get_fan_speed(DeviceConfig device_config) 
{
    if (device_config.EMC2101) {
        return EMC2101_get_fan_speed();
    }
    if (device_config.EMC2103) {
        return EMC2103_get_fan_speed();
    }
    if (device_config.EMC2302) {
        return EMC2302_get_fan_speed(0);
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
            return EMC2101_get_internal_temp() + GLOBAL_STATE->DEVICE_CONFIG.temp_offset;
        } else {
            return EMC2101_get_external_temp();
        }
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2103) {
        return EMC2103_get_external_temp() + GLOBAL_STATE->DEVICE_CONFIG.temp_offset;
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.EMC2302) {
        return (TMP1075_1_read_temperature() + TMP1075_2_read_temperature()) / 2 + GLOBAL_STATE->DEVICE_CONFIG.temp_offset;
    }
    return -1;
}

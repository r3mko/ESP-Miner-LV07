#ifndef TMP1075_LV07_H_
#define TMP1075_LV07_H_

#include "i2c_bitaxe.h"

// #define TMP1075_LV07_I2CADDR_DEFAULT 0x4A and 0x4B  ///< TMP1075 i2c address
#define TMP1075_LV07_TEMP_REG 0x00                     ///< Temperature register
#define TMP1075_LV07_CONFIG_REG 0x01                   ///< Configuration register
#define TMP1075_LV07_LOW_LIMIT 0x02                    ///< Low limit register
#define TMP1075_LV07_HIGH_LIMIT 0x03                   ///< High limit register
#define TMP1075_LV07_DEVICE_ID 0x0F                    ///< Device ID register

typedef struct {
    i2c_master_dev_handle_t dev_handle;
    const char             *TAG;
} tmp1075_t;

esp_err_t TMP1075_LV07_init(tmp1075_t *sensor, uint8_t i2c_address, const char *TAG, int temp_offset_param);
float TMP1075_LV07_read_temperature(tmp1075_t *sensor);

#endif /* TMP1075_LV07_H_ */

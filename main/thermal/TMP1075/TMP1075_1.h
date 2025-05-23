#ifndef TMP1075_1_H_
#define TMP1075_1_H_

#define TMP1075_1_I2CADDR_DEFAULT 0x4A  ///< TMP1075 i2c address
#define TMP1075_1_TEMP_REG 0x00         ///< Temperature register
#define TMP1075_1_CONFIG_REG 0x01       ///< Configuration register
#define TMP1075_1_LOW_LIMIT 0x02        ///< Low limit register
#define TMP1075_1_HIGH_LIMIT 0x03       ///< High limit register
#define TMP1075_1_DEVICE_ID 0x0F        ///< Device ID register

esp_err_t TMP1075_1_init(void);
uint8_t TMP1075_1_read_temperature(void);

#endif /* TMP1075_1_H_ */

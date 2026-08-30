#ifndef TPS546_CONFIG_H_
#define TPS546_CONFIG_H_

#include <stdint.h>

#define TPS546_INIT_PHASE_SINGLE 0x00
#define TPS546_INIT_PHASE_MULTI  0xFF

#define TPS546_SINGLE_PHASE_STACK_CONFIG 0x0000
#define TPS546_DUAL_PHASE_STACK_CONFIG   0x0001
#define TPS546_FOUR_PHASE_STACK_CONFIG   0x0003

#define TPS546_SYNC_CONFIG_DISABLED    0x10
#define TPS546_SYNC_CONFIG_AUTO_DETECT 0xD0

#define TPS546_DEFAULT_FREQUENCY 650  /* KHz */

typedef struct TPS546_CONFIG
{
    /* Phase readout configuration */
    uint8_t TPS546_INIT_PHASE; /* phase register configuration */

    /* vin voltage */
    float TPS546_INIT_VIN_ON; /* V */
    float TPS546_INIT_VIN_OFF; /* V */
    float TPS546_INIT_VIN_UV_WARN_LIMIT; /* V */
    float TPS546_INIT_VIN_OV_FAULT_LIMIT; /* V */

    /* vout voltage */
    float TPS546_INIT_SCALE_LOOP; /* Voltage Scale factor */
    float TPS546_INIT_VOUT_MIN; /* V */
    float TPS546_INIT_VOUT_MAX; /* V */
    float TPS546_INIT_VOUT_COMMAND; /* V absolute value */

    /* iout current */
    float TPS546_INIT_IOUT_OC_WARN_LIMIT; /* A */
    float TPS546_INIT_IOUT_OC_FAULT_LIMIT; /* A */

    uint16_t TPS546_INIT_STACK_CONFIG; /* Stack configuration */
    uint8_t TPS546_INIT_SYNC_CONFIG; /* Sync configuration */
    uint8_t TPS546_INIT_COMPENSATION_CONFIG[5];
    uint16_t TPS546_INIT_FREQUENCY; /* Switch frequency in KHz */
} TPS546_CONFIG;

extern const TPS546_CONFIG TPS546_CONFIG_DEFAULT;
extern const TPS546_CONFIG TPS546_CONFIG_HEX;
extern const TPS546_CONFIG TPS546_CONFIG_GAMMA_TURBO;
extern const TPS546_CONFIG TPS546_CONFIG_NAJA_DUO;
extern const TPS546_CONFIG TPS546_CONFIG_GAMMA_HEX;
extern const TPS546_CONFIG TPS546_CONFIG_LV07;
extern const TPS546_CONFIG TPS546_CONFIG_LV08;
extern const TPS546_CONFIG TPS546_CONFIG_LV07_PRO;

#endif /* TPS546_CONFIG_H_ */

#ifndef TPS546_CONFIG_H_
#define TPS546_CONFIG_H_

#include <stdint.h>

typedef struct
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
  
} TPS546_CONFIG;

#endif /* TPS546_CONFIG_H_ */

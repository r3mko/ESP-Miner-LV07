#include "TPS546.h"
#include "TPS546_LV08.h"
#include "INA260.h"
#include "DS4432U.h"
#include "vcore.h"

#include "power.h"

void Power_get_output(GlobalState * GLOBAL_STATE, float * power_out, float * current_out)
{
    *current_out = 0.0f;
    *power_out   = 0.0f;

    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        *current_out = TPS546_get_iout() * 1000.0f;
        // The power reading from the TPS546 is only it's output power. So the rest of the Bitaxe power is not accounted for.
        *power_out   = TPS546_get_vout() * (*current_out) / 1000.0f;
        *power_out  += GLOBAL_STATE->DEVICE_CONFIG.family.power_offset;  // Add offset for the rest of the Bitaxe power. TODO: this better.
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546_LV08) {
        tps546_t *v0 = VCORE_get_vreg(0);
        tps546_t *v1 = VCORE_get_vreg(1);
        tps546_t *v2 = VCORE_get_vreg(2);

        float i0 = TPS546_LV08_get_iout(v0) * 1000.0f;
        float i1 = TPS546_LV08_get_iout(v1) * 1000.0f;
        float i2 = TPS546_LV08_get_iout(v2) * 1000.0f;

        *current_out = (i0 + i1 + i2) / 3.0f;

        // calculate regulator power (in milliwatts)
        float p0 = TPS546_LV08_get_vout(v0) * i0 / 1000.0f;
        float p1 = TPS546_LV08_get_vout(v1) * i1 / 1000.0f;
        float p2 = TPS546_LV08_get_vout(v2) * i2 / 1000.0f;

        // The power reading from the TPS546 is only it's output power. So the rest of the Bitaxe power is not accounted for.
        *power_out = p0 + p1 + p2 + GLOBAL_STATE->DEVICE_CONFIG.family.power_offset;
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.INA260) {
        *current_out = INA260_read_current();
        *power_out   = INA260_read_power() / 1000.0f;
    }
}

float Power_get_input_voltage(GlobalState * GLOBAL_STATE)
{
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        return TPS546_get_vin() * 1000.0;
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546_LV08) {
        tps546_t *v0 = VCORE_get_vreg(0);
        tps546_t *v1 = VCORE_get_vreg(1);
        tps546_t *v2 = VCORE_get_vreg(2);

        float v0_in = TPS546_LV08_get_vin(v0) * 1000.0f;
        float v1_in = TPS546_LV08_get_vin(v1) * 1000.0f;
        float v2_in = TPS546_LV08_get_vin(v2) * 1000.0f;

        return (v0_in + v1_in + v2_in) / 3.0f;
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.INA260) {
        return INA260_read_voltage();
    }
    
    return 0.0;
}

float Power_get_vreg_temp(GlobalState * GLOBAL_STATE)
{
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546) {
        return TPS546_get_temperature();
    }
    if (GLOBAL_STATE->DEVICE_CONFIG.TPS546_LV08) {
        tps546_t *v0 = VCORE_get_vreg(0);
        tps546_t *v1 = VCORE_get_vreg(1);
        tps546_t *v2 = VCORE_get_vreg(2);

        float t0 = TPS546_LV08_get_temperature(v0);
        float t1 = TPS546_LV08_get_temperature(v1);
        float t2 = TPS546_LV08_get_temperature(v2);

        return (t0 + t1 + t2) / 3.0f;
    }

    return 0.0;
}

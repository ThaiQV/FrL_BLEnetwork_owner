/*
 * power_meter_app.h
 *
 *  Created on: Oct 6, 2025
 *      Author: hoang
 */

#ifndef VENDOR_TBS_DEV_TBS_POWER_METER_APP_POWER_METER_APP_H_
#define VENDOR_TBS_DEV_TBS_POWER_METER_APP_POWER_METER_APP_H_

#define NUMBER_CHANNEL_POWERMETTER	3

typedef struct {
    float voltage;              //  (V)
    float current;              //  (A)
    double active_power;         // (W)
    float reactive_power;       // (VAR)
    float apparent_power;       //  (VA)
    double active_energy;       // (Wh)
    double reactive_energy;     // (VARh)
    uint32_t time;              //
    uint32_t timestamp;         // (ms)
} measurement_data_t;

typedef struct {
    double voltage_sum;
    double current_sum;
    double active_power_sum;
    double reactive_power_sum;
    double apparent_power_sum;
    uint32_t sample_count;
} accumulator_t;

typedef struct
{
	uint8_t id;
	measurement_data_t measurement;
	accumulator_t accumulator;
	uint32_t start_time;
	uint32_t last_update_time;
} pmt_data_context_t;

void power_meter_app_init(void);
void power_meter_app_loop(void);

void pmt_setcalib(uint8_t ch, int32_t calib_U, int32_t calib_I, int32_t calib_P);
void pmt_getcalib(uint8_t ch, int32_t *calib_U, int32_t *calib_I, int32_t *calib_P);
float pmt_read_U(uint8_t ch);
float pmt_read_I(uint8_t ch);
float pmt_read_P(uint8_t ch);
void pmt_print_info(uint8_t ch);

#endif /* VENDOR_TBS_DEV_TBS_POWER_METER_APP_POWER_METER_APP_H_ */

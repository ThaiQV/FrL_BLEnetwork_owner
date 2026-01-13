/*
 * power_meter_app.h
 *
 *  Created on: Oct 6, 2025
 *      Author: hoang
 */

#ifndef VENDOR_TBS_DEV_TBS_POWER_METER_APP_POWER_METER_APP_H_
#define VENDOR_TBS_DEV_TBS_POWER_METER_APP_POWER_METER_APP_H_

#ifndef MASTER_CORE
#ifdef POWER_METER_DEVICE

#define NUMBER_CHANNEL_POWERMETTER	3

// #define PMT_DEBUG_UART
#ifdef PMT_DEBUG_UART
#define PMT_LOGA(...)  print_uart(__VA_ARGS__)
#else
#define PMT_LOGA(...)  printf(__VA_ARGS__)
#endif
typedef struct {
    float voltage;              //  (V)
    float current;              //  (A)
    float power_factor;
    double active_power;         // (W)
    float reactive_power;       // (VAR)
    float apparent_power;       //  (VA)
    double active_energy;       // (Wh)
    double reactive_energy;     // (VARh)
    float iT;
    float uT;
    float pT;
    uint32_t time;
    uint32_t timems;              //
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
void power_meter_app_read(void);
void pmt_clear_energy(u8 chn);
void pmt_setcalib(uint8_t ch, float calib_U, float calib_I, float calib_P);
void pmt_getcalib(uint8_t ch, float *calib_U, float *calib_I, float *calib_P);
void pmt_setcalibr(uint8_t ch, uint16_t calib_U, uint16_t calib_I, uint16_t calib_P);
void pmt_getcalibr(uint8_t ch, float *calib_U, float *calib_I, float *calib_P);
float pmt_read_U(uint8_t ch);
float pmt_read_I(uint8_t ch);
float pmt_read_P(uint8_t ch);
void pmt_print_info(uint8_t ch);

#endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* VENDOR_TBS_DEV_TBS_POWER_METER_APP_POWER_METER_APP_H_ */

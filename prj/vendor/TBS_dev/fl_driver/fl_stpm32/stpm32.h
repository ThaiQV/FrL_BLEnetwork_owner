/***************************************************
 Library for getting energy-data out of a STPM3X.
 Converted to C with multi-instance support.
 
 Corrected to use stpm3x_define.h definitions.

 Benjamin Völker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 
 Corrected by: AI Assistant
 Date: 2025
 ****************************************************/

#ifndef STPM3X_H
#define STPM3X_H

#ifndef MASTER_CORE
// #ifdef POWER_METER_DEVICE

#include <stdint.h>
#include <stdbool.h>
#include "stpm32_define.h"

#define TEST_STR_SIZE 300

typedef struct {
    double active;
    double fundamental;
    double reactive;
    double apparent;
    uint32_t valid_millis;
} stpm_energy_t;

typedef struct {
    uint32_t old_energy[4];
} stpm_energy_helper_t;

typedef struct {
    int reset_pin;
    int cs_pin;
    int syn_pin;
    int net_freq;

    float calibration[3][3];
    uint8_t crc_checksum;
    uint8_t address;
    bool auto_latch;
    bool crc_enabled;
    STPM3x_Gain_t gain1;
    STPM3x_Gain_t gain2;
    uint8_t read_buffer[10];
    char test_str[TEST_STR_SIZE];

    stpm_energy_t total_energy;
    stpm_energy_t ph1_energy;
    stpm_energy_t ph2_energy;

    stpm_energy_helper_t total_energy_hp;
    stpm_energy_helper_t ph1_energy_hp;
    stpm_energy_helper_t ph2_energy_hp;

    void * context;

    void (*log_func)(const char *msg, ...);

    // Platform specific SPI functions (must be provided by user)
    void (*spi_begin)(void);
    void (*spi_end)(void);
    void (*spi_begin_transaction)(void);
    void (*spi_end_transaction)(void);
    uint8_t (*spi_transfer)(uint8_t data);
    void (*spi_transfer_buffer)(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len);
    void (*pin_write)(int pin, bool state);
    void (*delay_us)(uint32_t us);
    void (*delay_ms)(uint32_t ms);
    uint32_t (*get_millis)(void);
} stpm_handle_t;

// Initialization
stpm_handle_t* stpm_create(int reset_pin, int cs_pin, int syn_pin, int fnet);
void stpm_destroy(stpm_handle_t *handle);
bool stpm_init(stpm_handle_t *handle);

// Configuration
void stpm_set_calibration(stpm_handle_t *handle, uint32_t cal_v, uint32_t cal_i);
void stpm_set_current_gain(stpm_handle_t *handle, uint8_t channel, STPM3x_Gain_t gain);
bool stpm_check_gain(stpm_handle_t *handle, uint8_t channel, uint8_t *buffer);
void stpm_auto_latch(stpm_handle_t *handle, bool enabled);
void stpm_crc(stpm_handle_t *handle, bool enabled);
void stpm_set_reference_frequency(stpm_handle_t *handle, STPM3x_Ref_Freq_t freq);
float stpm_read_period(stpm_handle_t *handle, uint8_t channel);
// Energy functions
void stpm_update_energy(stpm_handle_t *handle, uint8_t channel);
void stpm_reset_energies(stpm_handle_t *handle);
double stpm_read_total_active_energy(stpm_handle_t *handle);
double stpm_read_total_fundamental_energy(stpm_handle_t *handle);
double stpm_read_total_reactive_energy(stpm_handle_t *handle);
double stpm_read_total_apparent_energy(stpm_handle_t *handle);
double stpm_read_active_energy(stpm_handle_t *handle, uint8_t channel);
double stpm_read_fundamental_energy(stpm_handle_t *handle, uint8_t channel);
double stpm_read_reactive_energy(stpm_handle_t *handle, uint8_t channel);
double stpm_read_apparent_energy(stpm_handle_t *handle, uint8_t channel);
bool stpm_check_energy_ovf(stpm_handle_t *handle, uint8_t channel, char **raw_buffer);

// Power functions
void stpm_read_power(stpm_handle_t *handle, uint8_t channel, float *active,
						float *fundamental, float *reactive, float *apparent);
float stpm_read_power_factor(stpm_handle_t *handle, uint8_t channel);
double stpm_read_active_power(stpm_handle_t *handle, uint8_t channel);
float stpm_read_fundamental_power(stpm_handle_t *handle, uint8_t channel);
float stpm_read_reactive_power(stpm_handle_t *handle, uint8_t channel);
double stpm_read_apparent_rms_power(stpm_handle_t *handle, uint8_t channel);
float stpm_read_apparent_vectorial_power(stpm_handle_t *handle, uint8_t channel);
float stpm_read_momentary_active_power(stpm_handle_t *handle, uint8_t channel);
float stpm_read_momentary_fundamental_power(stpm_handle_t *handle, uint8_t channel);

// Voltage and Current functions
void stpm_read_all(stpm_handle_t *handle, uint8_t channel, float *voltage,
					float *current, float *active, float *reactive);
void stpm_read_voltage_and_current(stpm_handle_t *handle, uint8_t channel,
		float *voltage, float *current);
float stpm_read_voltage(stpm_handle_t *handle, uint8_t channel);
float stpm_read_current(stpm_handle_t *handle, uint8_t channel);
float stpm_read_fundamental_voltage(stpm_handle_t *handle, uint8_t channel);
float stpm_read_fundamental_current(stpm_handle_t *handle, uint8_t channel);
void stpm_read_rms_voltage_and_current(stpm_handle_t *handle, uint8_t channel,
                                       float *voltage, float *current);
float stpm_read_rms_voltage(stpm_handle_t *handle, uint8_t channel);
float stpm_read_rms_current(stpm_handle_t *handle, uint8_t channel);

// Additional measurements
void stpm_read_voltage_sag_and_swell_time(stpm_handle_t *handle, uint8_t channel,
                                          uint32_t *sag, uint32_t *swell);
void stpm_read_current_phase_and_swell_time(stpm_handle_t *handle, uint8_t channel,
                                            uint32_t *phase, uint32_t *swell);
void stpm_read_periods(stpm_handle_t *handle, uint32_t *ch1, uint32_t *ch2);

// Utility functions
void stpm_latch_reg(stpm_handle_t *handle);
char* stpm_register_to_str(stpm_handle_t *handle, uint8_t *frame);

void stpm32_read_frame(stpm_handle_t *handle, uint8_t address, uint8_t *buffer);
void stpm32_send_frame(stpm_handle_t *handle, uint8_t read_add, uint8_t write_add,
                       uint8_t data_lsb, uint8_t data_msb);

void stpm_update_calib(stpm_handle_t *handle, uint8_t channel, uint16_t calib_V, uint16_t calib_C);

// #endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
#endif // STPM3X_H

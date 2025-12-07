/***************************************************
 Library for getting energy-data out of a STPM3X.
 Converted to C with multi-instance support.
 
 Corrected to use stpm3x_define.h definitions.

 Benjamin VÃ¶lker, voelkerb@me.com
 Embedded Systems
 University of Freiburg, Institute of Informatik
 
 Corrected by: AI Assistant
 Date: 2025
 ****************************************************/

#include "stpm32_define.h"
#include "stpm32.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *energy_names[] = {"Active", "Fundamental", "Reactive", "Apparent"};

// Internal helper functions
static void latch(stpm_handle_t *handle);
static void read_frame(stpm_handle_t *handle, uint8_t address, uint8_t *buffer);
static void send_frame(stpm_handle_t *handle, uint8_t read_add, uint8_t write_add,
                       uint8_t data_lsb, uint8_t data_msb);
static void send_frame_crc(stpm_handle_t *handle, uint8_t read_add, uint8_t write_add,
                           uint8_t data_lsb, uint8_t data_msb);
static uint8_t calc_crc8(stpm_handle_t *handle, uint8_t *buf);
static void crc8_calc(stpm_handle_t *handle, uint8_t data);
static bool init_stpm34(stpm_handle_t *handle);

// Conversion functions
static inline float calc_period(uint16_t value);
static inline float calc_power(int32_t value);
static inline double calc_energy(uint32_t value);
static inline float calc_current_i32(int32_t value);
static inline float calc_current_u32(uint32_t value);
static inline float calc_volt_i32(int32_t value);
static inline float calc_volt_u16(uint16_t value);
// Buffer extraction functions
static inline int32_t buffer_0to32(uint8_t *buffer);
static inline int32_t buffer_0to28(uint8_t *buffer);
static inline int32_t buffer_0to23(uint8_t *buffer);
static inline uint32_t buffer_15to32(uint8_t *buffer);
static inline uint16_t buffer_16to30(uint8_t *buffer);
static inline uint16_t buffer_0to14(uint8_t *buffer);
static inline uint16_t buffer_0to11(uint8_t *buffer);
static inline uint16_t buffer_16to27(uint8_t *buffer);

// Create and destroy handle
stpm_handle_t* stpm_create(int reset_pin, int cs_pin, int syn_pin, int fnet) {
    stpm_handle_t *handle = (stpm_handle_t*)malloc(sizeof(stpm_handle_t));
    if (!handle) return NULL;

    memset(handle, 0, sizeof(stpm_handle_t));

    handle->reset_pin = reset_pin;
    handle->cs_pin = cs_pin;
    handle->syn_pin = syn_pin;
    handle->net_freq = fnet;
    handle->auto_latch = false;
    handle->crc_enabled = true;

    for (uint8_t i = 0; i < 3; i++) {
        handle->calibration[i][0] = 1;
        handle->calibration[i][1] = 1;
        handle->calibration[i][2] = 1;
    }

    handle->log_func = NULL;

    return handle;
}

void stpm_destroy(stpm_handle_t *handle) {
    if (handle) {
        free(handle);
    }
}

bool stpm_init(stpm_handle_t *handle) {
    if (!handle) return false;
    if (!handle->spi_begin || !handle->spi_transfer || !handle->pin_write ||
        !handle->delay_ms || !handle->delay_us || !handle->get_millis) {
        return false;
    }

    // Configure pins as outputs and perform reset sequence
    handle->pin_write(handle->cs_pin, false);
    if (handle->syn_pin != -1) handle->pin_write(handle->syn_pin, true);
//    handle->pin_write(handle->reset_pin, false);
//    handle->delay_ms(35);
//    handle->pin_write(handle->reset_pin, true);
//    handle->delay_ms(35);
    handle->pin_write(handle->cs_pin, true);
    handle->delay_ms(2);

    // Init sequence by toggling SYN pin 3 times
    if (handle->syn_pin != -1) {
        for (size_t i = 0; i < 3; i++) {
            handle->pin_write(handle->syn_pin, false);
            handle->delay_ms(2);
            handle->pin_write(handle->syn_pin, true);
            handle->delay_ms(2);
        }
    }

    handle->delay_ms(2);
    handle->pin_write(handle->cs_pin, false);
    handle->delay_ms(5);
    handle->pin_write(handle->cs_pin, true);

    handle->spi_begin();

    // bool success = init_stpm34(handle);
    return true;
}
// extern void print_uart(const char *fmt, ...);
static bool init_stpm34(stpm_handle_t *handle) {
    uint8_t data_lsb, data_msb;
    DSP_CR1_LSW_bits_t dsp_cr1_lsw;
	DSP_CR1_MSW_bits_t dsp_cr1_msw;
	DSP_CR2_LSW_bits_t dsp_cr2_lsw;
	DSP_CR2_MSW_bits_t dsp_cr2_msw;
	DFE_CR1_LSW_bits_t dfe_cr1_lsw;
	DFE_CR1_MSW_bits_t dfe_cr1_msw;
//	DFE_CR2_LSW_bits_t dfe_cr2_lsw;
//	DFE_CR2_MSW_bits_t dfe_cr2_msw;
	uint8_t buff[5];
	// read_frame(handle, DSP_CR1_Address, buff);
	// print_uart("%02x%02x%02x%02x\n", buff[0], buff[1], buff[2], buff[3] );

    // Configure DSP Control Register 1 LSW
    memset(&dsp_cr1_lsw, 0, sizeof(dsp_cr1_lsw));
    dsp_cr1_lsw.ENVREF1 = 1;  // Enable internal voltage reference
    dsp_cr1_lsw.TC1 = 0x02;   // Temperature compensation

    data_lsb = dsp_cr1_lsw.LSB;
    data_msb = dsp_cr1_lsw.MSB;
    send_frame_crc(handle, DSP_CR1_Address, DSP_CR1_Address, data_lsb, data_msb);

    // Configure DSP Control Register 1 MSW
    memset(&dsp_cr1_msw, 0, sizeof(dsp_cr1_msw));
    dsp_cr1_msw.BHPFV1 = 1;   // Enable HPF voltage
    dsp_cr1_msw.BHPFC1 = 1;   // Enable HPF current
    dsp_cr1_msw.LPW1 = 0x04;  // LED speed divider
    dsp_cr1_msw.AEM1 = 1;     // Enable Active Energy accumulation
    dsp_cr1_msw.APM1 = 1;     // Enable Active Power calculation
    data_lsb = dsp_cr1_msw.LSB;
    data_msb = dsp_cr1_msw.MSB;
    send_frame_crc(handle, DSP_CR1_Address, DSP_CR1_Address + 1, data_lsb, data_msb);

    // Configure DFE Control Register 1 -
    memset(&dfe_cr1_lsw, 0, sizeof(dfe_cr1_lsw));
    dfe_cr1_msw.ENC1 = 1;  // Enable current channel 1

    data_lsb = dfe_cr1_lsw.LSB;
    data_msb = dfe_cr1_lsw.MSB;
    send_frame_crc(handle, DFE_CR1_Address, DFE_CR1_Address, data_lsb, data_msb);

    memset(&dfe_cr1_msw, 0, sizeof(dfe_cr1_msw));
    dfe_cr1_lsw.ENV1 = 1;   // Enable voltage measurement 1

    data_lsb = dfe_cr1_msw.LSB;
    data_msb = dfe_cr1_msw.MSB;
    send_frame_crc(handle, DFE_CR1_Address, DFE_CR1_Address + 1, data_lsb, data_msb);


    // Configure DSP Control Register 2 LSW
    memset(&dsp_cr2_lsw, 0, sizeof(dsp_cr2_lsw));
    dsp_cr2_lsw.ENVREF2 = 1;  // Enable internal voltage reference
    dsp_cr2_lsw.TC2 = 0x02;   // Temperature compensation
    data_lsb = dsp_cr2_lsw.LSB;
    data_msb = dsp_cr2_lsw.MSB;
    send_frame_crc(handle, DSP_CR2_Address, DSP_CR2_Address, data_lsb, data_msb);

    // Configure DSP Control Register 2 MSW
    memset(&dsp_cr2_msw, 0, sizeof(dsp_cr2_msw));
    dsp_cr2_msw.BHPFV2 = 0;   // Enable HPF voltage
    dsp_cr2_msw.BHPFC2 = 0;   // Enable HPF current
//    dsp_cr2_msw.BLPFV2 = 1;   // Bypass LPF voltage (wideband)
//    dsp_cr2_msw.BLPFC2 = 1;   // Bypass LPF current (wideband)
    dsp_cr2_msw.LPW2 = 0x04;  // LED speed divider
    data_lsb = dsp_cr2_msw.LSB;
    data_msb = dsp_cr2_msw.MSB;
    send_frame_crc(handle, DSP_CR2_Address, DSP_CR2_Address + 1, data_lsb, data_msb);

    // Set current gain for both channels
    stpm_set_current_gain(handle, 1, STPM3X_GAIN_16X);
    stpm_set_current_gain(handle, 2, STPM3X_GAIN_16X);

    // Verify STPM is working by checking gain settings
    read_frame(handle, DFE_CR1_Address, handle->read_buffer);
    bool success = stpm_check_gain(handle, 1, handle->read_buffer);

    read_frame(handle, DFE_CR2_Address, handle->read_buffer);
    success = success && stpm_check_gain(handle, 2, handle->read_buffer);

    // Configure latch and CRC
    stpm_auto_latch(handle, false);
    stpm_crc(handle, false);

    return success;
}

void stpm_set_calibration(stpm_handle_t *handle, uint32_t cal_v, uint32_t cal_i) {
    if (!handle) return;

    for (uint8_t i = 0; i < 3; i++) {
        handle->calibration[i][0] = cal_v;
        handle->calibration[i][1] = cal_i;
    }
}

void stpm_crc(stpm_handle_t *handle, bool enabled) {
    if (!handle) return;
    if (handle->crc_enabled == enabled) return;

    US_REG1_LSW_bits_t us_reg1_lsw;
    memset(&us_reg1_lsw, 0, sizeof(us_reg1_lsw));
    us_reg1_lsw.CRC_EN = enabled ? 1 : 0;

    if (!enabled) {
        send_frame_crc(handle, US_REG1_Address, US_REG1_Address, 
                      us_reg1_lsw.LSB, us_reg1_lsw.MSB);
    } else {
        send_frame(handle, US_REG1_Address, US_REG1_Address, 
                  us_reg1_lsw.LSB, us_reg1_lsw.MSB);
    }

    handle->crc_enabled = enabled;
}

void stpm_auto_latch(stpm_handle_t *handle, bool enabled) {
    if (!handle) return;

    DSP_CR3_MSW_bits_t dsp_cr3_msw;
    memset(&dsp_cr3_msw, 0, sizeof(dsp_cr3_msw));

    handle->auto_latch = enabled;

    if (handle->auto_latch) {
        dsp_cr3_msw.SW_Auto_Latch = 1;
        dsp_cr3_msw.SW_Latch1 = 0;
        dsp_cr3_msw.SW_Latch2 = 0;
    } else {
        dsp_cr3_msw.SW_Auto_Latch = 0;
        dsp_cr3_msw.SW_Latch1 = 1;
        dsp_cr3_msw.SW_Latch2 = 1;
    }

    if (handle->crc_enabled) {
        send_frame_crc(handle, DSP_CR3_Address, DSP_CR3_Address + 1, 
                      dsp_cr3_msw.LSB, dsp_cr3_msw.MSB);
    } else {
        send_frame(handle, DSP_CR3_Address, DSP_CR3_Address + 1, 
                  dsp_cr3_msw.LSB, dsp_cr3_msw.MSB);
    }
}

void stpm_set_reference_frequency(stpm_handle_t *handle, STPM3x_Ref_Freq_t freq) {
    if (!handle) return;

    DSP_CR3_MSW_bits_t dsp_cr3_msw;
    read_frame(handle, DSP_CR3_Address + 1, handle->read_buffer);
    
    memset(&dsp_cr3_msw, 0, sizeof(dsp_cr3_msw));
    dsp_cr3_msw.LSB = handle->read_buffer[0];
    dsp_cr3_msw.MSB = handle->read_buffer[1];
    dsp_cr3_msw.REFFREQ = freq;

    if (handle->crc_enabled) {
        send_frame_crc(handle, DSP_CR3_Address, DSP_CR3_Address + 1, 
                      dsp_cr3_msw.LSB, dsp_cr3_msw.MSB);
    } else {
        send_frame(handle, DSP_CR3_Address, DSP_CR3_Address + 1, 
                  dsp_cr3_msw.LSB, dsp_cr3_msw.MSB);
    }
}

void stpm_set_current_gain(stpm_handle_t *handle, uint8_t channel, STPM3x_Gain_t gain) {
    if (!handle) return;
    if (channel != 1 && channel != 2) return;

    uint8_t read_add, write_add, data_lsb, data_msb;

    if (channel == 1) {
        DFE_CR1_MSW_bits_t dfe_cr1_msw;
        read_frame(handle, DFE_CR1_Address, handle->read_buffer);
        
        read_add = DSP_CR1_Address;
        write_add = DFE_CR1_Address + 1;
        
        memset(&dfe_cr1_msw, 0, sizeof(dfe_cr1_msw));
        dfe_cr1_msw.LSB = handle->read_buffer[2];
        dfe_cr1_msw.MSB = handle->read_buffer[3];
        dfe_cr1_msw.GAIN1 = gain;
        dfe_cr1_msw.ENC1 = 1;  // Enable current channel
        
        handle->gain1 = gain;
        data_lsb = dfe_cr1_msw.LSB;
        data_msb = dfe_cr1_msw.MSB;
    } else {
        DFE_CR2_MSW_bits_t dfe_cr2_msw;
        read_frame(handle, DFE_CR2_Address, handle->read_buffer);
        
        read_add = DSP_CR1_Address;
        write_add = DFE_CR2_Address + 1;
        
        memset(&dfe_cr2_msw, 0, sizeof(dfe_cr2_msw));
        dfe_cr2_msw.LSB = handle->read_buffer[2];
        dfe_cr2_msw.MSB = handle->read_buffer[3];
        dfe_cr2_msw.GAIN2 = gain;
        dfe_cr2_msw.ENC2 = 1;  // Enable current channel
        
        handle->gain2 = gain;
        data_lsb = dfe_cr2_msw.LSB;
        data_msb = dfe_cr2_msw.MSB;
    }

    send_frame_crc(handle, read_add, write_add, data_lsb, data_msb);
}

bool stpm_check_gain(stpm_handle_t *handle, uint8_t channel, uint8_t *buffer) {
    if (!handle) return false;
    if (channel != 1 && channel != 2) return false;

    if (channel == 1) {
        DFE_CR1_MSW_bits_t dfe_cr1_msw;
        memset(&dfe_cr1_msw, 0, sizeof(dfe_cr1_msw));
        dfe_cr1_msw.LSB = buffer[2];
        dfe_cr1_msw.MSB = buffer[3];
        if (dfe_cr1_msw.GAIN1 != handle->gain1) return false;
    } else {
        DFE_CR2_MSW_bits_t dfe_cr2_msw;
        memset(&dfe_cr2_msw, 0, sizeof(dfe_cr2_msw));
        dfe_cr2_msw.LSB = buffer[2];
        dfe_cr2_msw.MSB = buffer[3];
        if (dfe_cr2_msw.GAIN2 != handle->gain2) return false;
    }

    return true;
}

float stpm_read_period(stpm_handle_t *handle, uint8_t channel)
{
    if (!handle || (channel != 1 && channel != 2)) return;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = DSP_REG1_Address;
    read_frame(handle, address, handle->read_buffer);
    float frequency;
    if (channel == 1)
    {
       frequency = buffer_0to11(handle->read_buffer);
    }

    return frequency = 1000000.0 / ( frequency * 8.0);
}

void stpm_update_energy(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || channel > 2) return;
    if (!handle->auto_latch) latch(handle);

    int8_t address;
    stpm_energy_t *energy;
    stpm_energy_helper_t *energy_helper;

    if (channel == 0) {
        address = Total_Active_Energy_Address;
        energy = &handle->total_energy;
        energy_helper = &handle->total_energy_hp;
    } else if (channel == 1) {
        address = PH1_Active_Energy_Address;
        energy = &handle->ph1_energy;
        energy_helper = &handle->ph1_energy_hp;
    } else {
        address = PH2_Active_Energy_Address;
        energy = &handle->ph2_energy;
        energy_helper = &handle->ph2_energy_hp;
    }

    uint32_t my_energies[4] = {0};

    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);
    handle->spi_transfer(address);
    handle->spi_transfer(0xff);
    handle->spi_transfer(0xff);
    handle->spi_transfer(0xff);
    handle->pin_write(handle->cs_pin, true);
    handle->delay_us(4);
    handle->pin_write(handle->cs_pin, false);

    // Read 4 consecutive energy registers (Active, Fundamental, Reactive, Apparent)
    for (int i = 0; i < 4; i++) {
        handle->read_buffer[0] = handle->spi_transfer(0xff);
        handle->read_buffer[1] = handle->spi_transfer(0xff);
        handle->read_buffer[2] = handle->spi_transfer(0xff);
        handle->read_buffer[3] = handle->spi_transfer(0xff);
        my_energies[i] = (((handle->read_buffer[3] << 24) | (handle->read_buffer[2] << 16)) |
                         (handle->read_buffer[1] << 8)) | handle->read_buffer[0];
    }

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();

    uint32_t now = handle->get_millis();
    double *update_energies[4] = {&energy->active, &energy->fundamental,
                                  &energy->reactive, &energy->apparent};

    for (int i = 0; i < 4; i++) {
        uint32_t delta = my_energies[i] - energy_helper->old_energy[i];
        double e = calc_energy(delta);
        uint32_t delta_t = now - energy->valid_millis;
        double p = e * 1000.0 * 3600.0 / (double)delta_t;

        // Check for positive energy change
        if (delta < (uint32_t)2147483648) {
            if (p < NOISE_POWER || p > MAX_POWER) e = 0;
        } else {
            // Handle negative energy (wrap-around)
            delta = energy_helper->old_energy[i] - my_energies[i];
            e = -1.0 * calc_energy(delta);
            p = e * 1000.0 * 3600.0 / (double)delta_t;
            if (p > -2 * NOISE_POWER || p < -MAX_POWER) e = 0;
        }

        *update_energies[i] += e;
        energy_helper->old_energy[i] = my_energies[i];
    }

    energy->valid_millis = now;
}

void stpm_reset_energies(stpm_handle_t *handle) {
    if (!handle) return;

    uint32_t now = handle->get_millis();
    memset(&handle->total_energy, 0, sizeof(stpm_energy_t));
    memset(&handle->ph1_energy, 0, sizeof(stpm_energy_t));
    memset(&handle->ph2_energy, 0, sizeof(stpm_energy_t));

    handle->total_energy.valid_millis = now;
    handle->ph1_energy.valid_millis = now;
    handle->ph2_energy.valid_millis = now;
}

double stpm_read_active_energy(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || channel > 2) return -1.0;

    stpm_energy_t *energy;
    if (channel == 0) energy = &handle->total_energy;
    else if (channel == 1) energy = &handle->ph1_energy;
    else energy = &handle->ph2_energy;

    if (handle->get_millis() - energy->valid_millis > ENERGY_UPDATE_MS) {
        stpm_update_energy(handle, channel);
    }

    return energy->active;
}

double stpm_read_fundamental_energy(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || channel > 2) return -1.0;

    stpm_energy_t *energy;
    if (channel == 0) energy = &handle->total_energy;
    else if (channel == 1) energy = &handle->ph1_energy;
    else energy = &handle->ph2_energy;

    if (handle->get_millis() - energy->valid_millis > ENERGY_UPDATE_MS) {
        stpm_update_energy(handle, channel);
    }

    return energy->fundamental;
}

double stpm_read_reactive_energy(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || channel > 2) return -1.0;

    stpm_energy_t *energy;
    if (channel == 0) energy = &handle->total_energy;
    else if (channel == 1) energy = &handle->ph1_energy;
    else energy = &handle->ph2_energy;

    if (handle->get_millis() - energy->valid_millis > ENERGY_UPDATE_MS) {
        stpm_update_energy(handle, channel);
    }

    return energy->reactive;
}

double stpm_read_apparent_energy(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || channel > 2) return -1.0;

    stpm_energy_t *energy;
    if (channel == 0) energy = &handle->total_energy;
    else if (channel == 1) energy = &handle->ph1_energy;
    else energy = &handle->ph2_energy;

    if (handle->get_millis() - energy->valid_millis > ENERGY_UPDATE_MS) {
        stpm_update_energy(handle, channel);
    }

    return energy->apparent;
}

double stpm_read_total_active_energy(stpm_handle_t *handle) {
    return stpm_read_active_energy(handle, 0);
}

double stpm_read_total_fundamental_energy(stpm_handle_t *handle) {
    return stpm_read_fundamental_energy(handle, 0);
}

double stpm_read_total_reactive_energy(stpm_handle_t *handle) {
    return stpm_read_reactive_energy(handle, 0);
}

double stpm_read_total_apparent_energy(stpm_handle_t *handle) {
    return stpm_read_apparent_energy(handle, 0);
}

double stpm_read_active_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Active_Power_Address : PH2_Active_Power_Address;
    read_frame(handle, address, handle->read_buffer);
    int32_t value = buffer_0to28(handle->read_buffer);

    return (double) ((value) / handle->calibration[channel][2]);
}

float stpm_read_fundamental_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Fundamental_Power_Address : PH2_Fundamental_Power_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_power(buffer_0to28(handle->read_buffer)) *
           handle->calibration[channel][0] * handle->calibration[channel][1];
}

float stpm_read_reactive_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Reactive_Power_Address : PH2_Reactive_Power_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_power(buffer_0to28(handle->read_buffer)) *
           handle->calibration[channel][0] * handle->calibration[channel][1];
}

float stpm_read_apparent_rms_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Apparent_RMS_Power_Address : PH2_Apparent_RMS_Power_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_power(buffer_0to28(handle->read_buffer)) *
           handle->calibration[channel][0] * handle->calibration[channel][1];
}

float stpm_read_apparent_vectorial_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Apparent_Vec_Power_Address : PH2_Apparent_Vec_Power_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_power(buffer_0to28(handle->read_buffer)) *
           handle->calibration[channel][0] * handle->calibration[channel][1];
}

float stpm_read_momentary_active_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Momentary_Active_Power_Address : PH2_Momentary_Active_Power_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_power(buffer_0to28(handle->read_buffer)) *
           handle->calibration[channel][0] * handle->calibration[channel][1];
}

float stpm_read_momentary_fundamental_power(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Momentary_Fund_Power_Address : PH2_Momentary_Fund_Power_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_power(buffer_0to28(handle->read_buffer)) *
           handle->calibration[channel][0] * handle->calibration[channel][1];
}

void stpm_read_power(stpm_handle_t *handle, uint8_t channel, float *active,
                     float *fundamental, float *reactive, float *apparent) {
    if (!handle || (channel != 1 && channel != 2)) return;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Active_Power_Address : PH2_Active_Power_Address;

    send_frame(handle, address, 0xff, 0xff, 0xff);
    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);

    // Read Active Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *active = calc_power(buffer_0to28(handle->read_buffer)) *
              handle->calibration[channel][0] * handle->calibration[channel][1];

    // Read Fundamental Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *fundamental = calc_power(buffer_0to28(handle->read_buffer)) *
                   handle->calibration[channel][0] * handle->calibration[channel][1];

    // Read Reactive Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *reactive = calc_power(buffer_0to28(handle->read_buffer)) *
                handle->calibration[channel][0] * handle->calibration[channel][1];

    // Read Apparent RMS Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *apparent = calc_power(buffer_0to28(handle->read_buffer)) *
                handle->calibration[channel][0] * handle->calibration[channel][1];

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
}

float stpm_read_voltage(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? V1_Data_Address : V2_Data_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_volt_i32(buffer_0to23(handle->read_buffer)) *
           handle->calibration[channel][0];
}

float stpm_read_current(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? C1_Data_Address : C2_Data_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_current_i32(buffer_0to23(handle->read_buffer)) *
           handle->calibration[channel][1];
}

void stpm_read_voltage_and_current(stpm_handle_t *handle, uint8_t channel,
                                   float *voltage, float *current) {
    if (!handle || (channel != 1 && channel != 2)) return;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? V1_Data_Address : V2_Data_Address;

    send_frame(handle, address, 0xff, 0xff, 0xff);
    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);

    // Read Voltage
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *voltage = calc_volt_i32(buffer_0to32(handle->read_buffer)) *
               handle->calibration[channel][0];

    // Read Current
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *current = calc_current_i32(buffer_0to32(handle->read_buffer)) *
               handle->calibration[channel][1];

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
}

void stpm_read_all(stpm_handle_t *handle, uint8_t channel, float *voltage,
                   float *current, float *active, float *reactive) {
    if (!handle || (channel != 1 && channel != 2)) return;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? PH1_Active_Power_Address : PH2_Active_Power_Address;

    send_frame(handle, address, 0xff, 0xff, 0xff);
    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);

    // Read Active Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *active = calc_power(buffer_0to28(handle->read_buffer)) *
              handle->calibration[channel][0] * handle->calibration[channel][1];

    // Skip Fundamental Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);

    // Read Reactive Power
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *reactive = calc_power(buffer_0to28(handle->read_buffer)) *
                handle->calibration[channel][0] * handle->calibration[channel][1];

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();

    // Now read voltage and current
    address = (channel == 1) ? V1_Data_Address : V2_Data_Address;
    send_frame(handle, address, 0xff, 0xff, 0xff);
    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);

    // Read Voltage
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *voltage = calc_volt_i32(buffer_0to32(handle->read_buffer)) *
               handle->calibration[channel][0];

    // Read Current
    handle->read_buffer[0] = handle->spi_transfer(0xff);
    handle->read_buffer[1] = handle->spi_transfer(0xff);
    handle->read_buffer[2] = handle->spi_transfer(0xff);
    handle->read_buffer[3] = handle->spi_transfer(0xff);
    *current = calc_current_i32(buffer_0to32(handle->read_buffer)) *
               handle->calibration[channel][1];

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
}

float stpm_read_fundamental_voltage(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? V1_Fund_Address : V2_Fund_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_volt_i32(buffer_0to32(handle->read_buffer)) *
           handle->calibration[channel][0];
}

float stpm_read_fundamental_current(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? C1_Fund_Address : C2_Fund_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_volt_i32(buffer_0to32(handle->read_buffer)) *
           handle->calibration[channel][0];
}

void stpm_read_rms_voltage_and_current(stpm_handle_t *handle, uint8_t channel,
                                       float *voltage, float *current) {
    if (!handle || (channel != 1 && channel != 2)) return;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? DSP_REG14_Address : DSP_REG15_Address;
    read_frame(handle, address, handle->read_buffer);

    *voltage = (float) (((float)buffer_0to14(handle->read_buffer)) / (float)handle->calibration[channel][0]);
    *current = (float) (((float)buffer_15to32(handle->read_buffer)) /(float)handle->calibration[channel][1]);
}

float stpm_read_rms_voltage(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? DSP_REG14_Address : DSP_REG15_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_volt_u16(buffer_0to14(handle->read_buffer)) *
           handle->calibration[channel][0];
}

float stpm_read_rms_current(stpm_handle_t *handle, uint8_t channel) {
    if (!handle || (channel != 1 && channel != 2)) return 0;
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? DSP_REG14_Address : DSP_REG15_Address;
    read_frame(handle, address, handle->read_buffer);

    return calc_current_u32(buffer_15to32(handle->read_buffer)) *
           handle->calibration[channel][1];
}

void stpm_read_voltage_sag_and_swell_time(stpm_handle_t *handle, uint8_t channel,
                                          uint32_t *sag, uint32_t *swell) {
    if (!handle || (channel != 1 && channel != 2)) {
        if (sag) *sag = 0;
        if (swell) *swell = 0;
        return;
    }
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? DSP_REG16_Address : DSP_REG18_Address;
    read_frame(handle, address, handle->read_buffer);

    *swell = (uint32_t)buffer_0to14(handle->read_buffer);
    *sag = (uint32_t)buffer_16to30(handle->read_buffer);
}

void stpm_read_current_phase_and_swell_time(stpm_handle_t *handle, uint8_t channel,
                                            uint32_t *phase, uint32_t *swell) {
    if (!handle || (channel != 1 && channel != 2)) {
        if (phase) *phase = 0;
        if (swell) *swell = 0;
        return;
    }
    if (!handle->auto_latch) latch(handle);

    uint8_t address = (channel == 1) ? DSP_REG17_Address : DSP_REG19_Address;
    read_frame(handle, address, handle->read_buffer);

    *swell = (uint32_t)buffer_0to14(handle->read_buffer);
    *phase = (uint32_t)buffer_16to27(handle->read_buffer) * handle->net_freq * 360.0f / 125000.0f;
}

void stpm_read_periods(stpm_handle_t *handle, uint32_t *ch1, uint32_t *ch2) {
    if (!handle) return;
    if (!handle->auto_latch) latch(handle);

    read_frame(handle, DSP_REG1_Address, handle->read_buffer);

    *ch1 = calc_period(buffer_0to11(handle->read_buffer));
    *ch2 = calc_period(buffer_16to27(handle->read_buffer));
}

void stpm_latch_reg(stpm_handle_t *handle) {
//    latch(handle);
	handle->delay_us(4);
    handle->pin_write(handle->syn_pin, false);
    handle->delay_us(4);
    handle->pin_write(handle->syn_pin, true);
    handle->delay_us(4);
}

char* stpm_register_to_str(stpm_handle_t *handle, uint8_t *frame) {
    if (!handle) return NULL;

    strncpy(handle->test_str, "", sizeof(handle->test_str));
    char *str = handle->test_str;
    int remaining = TEST_STR_SIZE;

    int written = snprintf(str, remaining, "|");
    str += written;
    remaining -= written;

    for (int8_t i = 31; i >= 0; i--) {
        written = snprintf(str, remaining, "%02i|", i);
        str += written;
        remaining -= written;
    }

    written = snprintf(str, remaining, "\n| ");
    str += written;
    remaining -= written;

    for (int8_t i = 3; i >= 0; i--) {
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x80) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x40) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x20) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x10) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x08) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x04) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x02) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
        written = snprintf(str, remaining, "%s| ", (frame[i] & 0x01) > 0 ? "1" : "0");
        str += written;
        remaining -= written;
    }

    return handle->test_str;
}

// Internal helper functions implementation

static inline void latch(stpm_handle_t *handle) {
    return;
#ifdef SPI_LATCH
    DSP_CR3_MSW_bits_t dsp_cr3_msw;
    memset(&dsp_cr3_msw, 0, sizeof(dsp_cr3_msw));
    dsp_cr3_msw.SW_Latch1 = 1;
    dsp_cr3_msw.SW_Latch2 = 1;
    send_frame(handle, DSP_CR3_Address, DSP_CR3_Address + 1, dsp_cr3_msw.LSB, dsp_cr3_msw.MSB);
#else
    handle->delay_us(4);
    handle->pin_write(handle->syn_pin, false);
    handle->delay_us(4);
    handle->pin_write(handle->syn_pin, true);
    handle->delay_us(4);
#endif
}

static void read_frame(stpm_handle_t *handle, uint8_t address, uint8_t *buffer) {
    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);
    handle->spi_transfer(address);
    handle->spi_transfer(0xff);
    handle->spi_transfer(0xff);
    handle->spi_transfer(0xff);
    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();

    handle->delay_us(4);

    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);

    for (uint8_t i = 0; i < 4; i++) {
        buffer[i] = handle->spi_transfer(0xff);
    }

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
}

static void send_frame(stpm_handle_t *handle, uint8_t read_add, uint8_t write_add,
                       uint8_t data_lsb, uint8_t data_msb) {
    uint8_t frame[STPM3x_FRAME_LEN];
    frame[0] = read_add;
    frame[1] = write_add;
    frame[2] = data_lsb;
    frame[3] = data_msb;

    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);
    handle->spi_transfer(frame[0]);
    handle->spi_transfer(frame[1]);
    handle->spi_transfer(frame[2]);
    handle->spi_transfer(frame[3]);
    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
    
    handle->delay_us(4);
}

static void send_frame_crc(stpm_handle_t *handle, uint8_t read_add, uint8_t write_add,
                           uint8_t data_lsb, uint8_t data_msb) {
    uint8_t frame[STPM3x_FRAME_LEN];

    frame[0] = read_add;
    frame[1] = write_add;
    frame[2] = data_lsb;
    frame[3] = data_msb;
    frame[4] = calc_crc8(handle, frame);

    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);
    handle->spi_transfer(frame[0]);
    handle->spi_transfer(frame[1]);
    handle->spi_transfer(frame[2]);
    handle->spi_transfer(frame[3]);
    handle->spi_transfer(frame[4]);
    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
    
    handle->delay_us(4);
}

static uint8_t calc_crc8(stpm_handle_t *handle, uint8_t *buf) {
    handle->crc_checksum = 0x00;
    for (uint8_t i = 0; i < STPM3x_FRAME_LEN - 1; i++) {
        crc8_calc(handle, buf[i]);
    }
    return handle->crc_checksum;
}

static void crc8_calc(stpm_handle_t *handle, uint8_t data) {
    uint8_t idx = 0;
    uint8_t temp;

    while (idx < 8) {
        temp = data ^ handle->crc_checksum;
        handle->crc_checksum <<= 1;
        if (temp & 0x80) {
            handle->crc_checksum ^= CRC_8;
        }
        data <<= 1;
        idx++;
    }
}

// Conversion functions
static inline float calc_period(uint16_t value) {
    return 1.0f / ((float)value * 8.0f / 1000000.0f);
}

static inline float calc_power(int32_t value) {
    return (float)value * 0.0001217f;
}

static inline double calc_energy(uint32_t value) {
    return (double)value * ENERGY_LSB;
}

static inline float calc_current_i32(int32_t value) {
    return (float)value * 0.003349213f;
}

static inline float calc_current_u32(uint32_t value) {
    return (float)value * 0.2143f;
}

static inline float calc_volt_i32(int32_t value) {
    return (float)value * 0.000138681f;
}

static inline float calc_volt_u16(uint16_t value) {
    return (float)value * 0.0354840440f;
}

// Buffer extraction functions
static inline int32_t buffer_0to32(uint8_t *buffer) {
    return (((buffer[3] << 24) | (buffer[2] << 16)) | (buffer[1] << 8) | buffer[0]);
}

static inline int32_t buffer_0to28(uint8_t *buffer) {
    // Ä�á»�c 4 bytes thÃ nh 32-bit
    int32_t value = (int32_t)(((uint32_t)buffer[3] << 24) | 
                              ((uint32_t)buffer[2] << 16) | 
                              ((uint32_t)buffer[1] << 8) | 
                              (uint32_t)buffer[0]);
    
    // Mask Ä‘á»ƒ láº¥y 28-bit
    value &= 0x0FFFFFFF;
    
    // Sign extension: náº¿u bit 27 = 1, má»Ÿ rá»™ng thÃ nh sá»‘ Ã¢m 32-bit
    if (value & 0x08000000) {
        value |= 0xF0000000;
    }
    
    return value;
}

static inline int32_t buffer_0to23(uint8_t *buffer) {
    return (((buffer[2] << 16)) | (buffer[1] << 8) | buffer[0] );
}

static inline uint32_t buffer_15to32(uint8_t *buffer) {
    return (((buffer[3] << 16) | (buffer[2] << 8)) | buffer[1]) >> 7;
}

static inline uint16_t buffer_16to30(uint8_t *buffer) {
    return ((buffer[3] & 0x7f) << 8) | buffer[2];
}

static inline uint16_t buffer_0to14(uint8_t *buffer) {
    return ((buffer[1] & 0x7f) << 8) | buffer[0];
}

static inline uint16_t buffer_0to11(uint8_t *buffer) {
    return ((buffer[1] & 0x0f) << 8) | buffer[0];
}

static inline uint16_t buffer_16to27(uint8_t *buffer) {
    return ((buffer[3] & 0x0f) << 8) | buffer[2];
}

void stpm32_read_frame(stpm_handle_t *handle, uint8_t address, uint8_t *buffer) {
    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);
    handle->spi_transfer(address);
    handle->spi_transfer(0xff);
    handle->spi_transfer(0xff);
    handle->spi_transfer(0xff);
    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();

    handle->delay_us(4);

    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);

    for (uint8_t i = 0; i < 4; i++) {
        buffer[i] = handle->spi_transfer(0xff);
    }

    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();
}

void stpm32_send_frame(stpm_handle_t *handle, uint8_t read_add, uint8_t write_add,
                       uint8_t data_lsb, uint8_t data_msb) {
    uint8_t frame[STPM3x_FRAME_LEN];
    frame[0] = read_add;
    frame[1] = write_add;
    frame[2] = data_lsb;
    frame[3] = data_msb;

    handle->spi_begin_transaction();
    handle->pin_write(handle->cs_pin, false);
    handle->spi_transfer(frame[0]);
    handle->spi_transfer(frame[1]);
    handle->spi_transfer(frame[2]);
    handle->spi_transfer(frame[3]);
    handle->pin_write(handle->cs_pin, true);
    handle->spi_end_transaction();

    handle->delay_us(4);
}

void stpm_update_calib(stpm_handle_t *handle, uint8_t channel, uint16_t calib_V, uint16_t calib_C)
{
    if (!handle) return;

    DSP_CR5_bits_t dsp_cr5;
    DSP_CR6_bits_t dsp_cr6;

    if(channel == 1)
    {
        read_frame(handle, DSP_CR5_Address, handle->read_buffer);
        dsp_cr5.LSW.LSB = handle->read_buffer[0];
        dsp_cr5.LSW.MSB = handle->read_buffer[1];
        dsp_cr5.MSW.LSB = handle->read_buffer[2];
        dsp_cr5.MSW.MSB = handle->read_buffer[3];
        printf("dsp_cr5: %08x\n", dsp_cr5);

        read_frame(handle, DSP_CR6_Address, handle->read_buffer);
        dsp_cr6.LSW.LSB = handle->read_buffer[0];
        dsp_cr6.LSW.MSB = handle->read_buffer[1];
        dsp_cr6.MSW.LSB = handle->read_buffer[2];
        dsp_cr6.MSW.MSB = handle->read_buffer[3];
        printf("dsp_cr6: %08x\n", dsp_cr6);

        dsp_cr5.CHV1 = calib_V;
        dsp_cr6.CHC1 = calib_C;

        if (handle->crc_enabled) {
            send_frame_crc(handle, DSP_CR5_Address, DSP_CR5_Address, dsp_cr5.LSW.LSB, dsp_cr5.LSW.MSB);
            send_frame_crc(handle, DSP_CR6_Address, DSP_CR6_Address, dsp_cr6.LSW.LSB, dsp_cr6.LSW.MSB);
        } else {
            send_frame(handle, DSP_CR5_Address, DSP_CR5_Address, dsp_cr5.LSW.LSB, dsp_cr5.LSW.MSB);
            send_frame(handle, DSP_CR6_Address, DSP_CR6_Address, dsp_cr6.LSW.LSB, dsp_cr6.LSW.MSB);
        }

        read_frame(handle, DSP_CR5_Address, handle->read_buffer);
        dsp_cr5.LSW.LSB = handle->read_buffer[0];
        dsp_cr5.LSW.MSB = handle->read_buffer[1];
        dsp_cr5.MSW.LSB = handle->read_buffer[2];
        dsp_cr5.MSW.MSB = handle->read_buffer[3];
        printf("dsp_cr5: %08x\n", dsp_cr5);

        read_frame(handle, DSP_CR6_Address, handle->read_buffer);
        dsp_cr6.LSW.LSB = handle->read_buffer[0];
        dsp_cr6.LSW.MSB = handle->read_buffer[1];
        dsp_cr6.MSW.LSB = handle->read_buffer[2];
        dsp_cr6.MSW.MSB = handle->read_buffer[3];
        printf("dsp_cr6: %08x\n", dsp_cr6);

    }
    else if(channel == 2)
    {

    }
    else
    {

    }
}

/*
 * power_meter_app.c
 *
 *  Created on: Oct 6, 2025
 *      Author: hoang
 */

#include"tl_common.h"
#include"../TBS_dev_config.h"
#include "vendor/FrL_network/fl_nwk_handler.h"
#include "vendor/FrL_network/fl_nwk_api.h"
#include "vendor/FrL_network/fl_nwk_database.h"
#include"../fl_driver/fl_stpm32/stpm32.h"
#include "power_meter_app.h"
#include "vendor/TBS_dev/TBS_dev_app/driver/common/msTick.h"
#include "uart_app/uart_app.h"
#include "uart_app/pmt_data.h"

#define PIN_CS1 GPIO_PE6 //GPIO_PE6
#define PIN_CS2 GPIO_PE5
#define PIN_CS3 GPIO_PE4
#define PIN_CS_ PIN_CS1
#define PIN_EN GPIO_PD3
#define PIN_SYN_ GPIO_PB1
#define PIN_MISO GPIO_PB2
#define PIN_MOSI GPIO_PB3

static pmt_data_context_t pmt_ctx[NUMBER_CHANNEL_POWERMETTER];
static stpm_handle_t* pmt_handle[NUMBER_CHANNEL_POWERMETTER];

extern tbs_device_powermeter_t G_POWER_METER;

u8 uart_rx_buff[256];
u32 uart_print_len = 0;
u8 uart_print_en = 0;


static void save_calib();
static void read_calib();


void uart_print_app(void)
{
    if( uart_print_en == 1)
    {
        printf("RX: %d: ", uart_print_len);
        for(int index = 0; index < uart_print_len; index++)
        {
            printf("%x ", uart_rx_buff[index]);

        }
        printf("\n");
        uart_print_en = 0;
    }
}
void read_stpm_data(stpm_handle_t *handle);
void update_energy(stpm_handle_t *handle);
void update_display(stpm_handle_t *handle, uint32_t current_time);
void stpm_monitoring_loop(stpm_handle_t **handle);

void print_hex(const char *label, uint8_t *data, uint8_t len);

void stpm32_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}

void stpm32_cs_low(void)
{
    gpio_set_level(PIN_CS_, 0);
}

void stpm32_cs_high(void)
{
    gpio_set_level(PIN_CS_, 1);
}

void stpm32_reset_low(void)
{
    
}

void stpm32_reset_high(void)
{
    
}

uint8_t stpm32_spi_transfer(uint8_t data)
{
    unsigned char ret = 0; 

    spi_tx_dma_dis(HSPI_MODULE);
    spi_rx_dma_dis(HSPI_MODULE);
    spi_tx_fifo_clr(HSPI_MODULE);
    spi_rx_fifo_clr(HSPI_MODULE);
    spi_tx_cnt(HSPI_MODULE, 1);
    spi_rx_cnt(HSPI_MODULE, 1);
    spi_set_transmode(HSPI_MODULE, SPI_MODE_WRITE_AND_READ);
    spi_set_cmd(HSPI_MODULE, 0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
    spi_write(HSPI_MODULE,&data,1);
    spi_read(HSPI_MODULE,&ret,1);
    while (spi_is_busy(HSPI_MODULE));

    return ret;
}

void stpm32_spi_transfer_buffer(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len)
{
     if (len == 0) return;

     stpm32_cs_low();

    
    // Disable DMA
    spi_tx_dma_dis(HSPI_MODULE);
    spi_rx_dma_dis(HSPI_MODULE);
    
    // Clear FIFO
    spi_tx_fifo_clr(HSPI_MODULE);
    spi_rx_fifo_clr(HSPI_MODULE);
    
    // Set count
    spi_tx_cnt(HSPI_MODULE, len);
    spi_rx_cnt(HSPI_MODULE, len);
    
    // Set mode
    spi_set_transmode(HSPI_MODULE, SPI_MODE_WRITE_AND_READ);
    spi_set_cmd(HSPI_MODULE, 0x00);
    
    // Write all bytes
    spi_write(HSPI_MODULE, tx_buf, len);
    
    // Read all bytes
    spi_read(HSPI_MODULE, rx_buf, len);
    
    while (spi_is_busy(HSPI_MODULE));

    stpm32_cs_high();

}


void print_hex(const char *label, uint8_t *data, uint8_t len)
{
    printf("%s: ", label);
    for (uint8_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

void pmt_spi_begin(void);
void pmt_spi_end(void);
void pmt_spi_begin_transaction(void);
void pmt_spi_end_transaction(void);
uint8_t pmt_spi_transfer(uint8_t data);
void pmt_pin_write(int pin, bool state);
void pmt_delay_us(uint32_t us);
void pmt_delay_ms(uint32_t ms);
uint32_t pmt_get_millis(void);

void pmt_spi_begin(void)
{

}

void pmt_spi_end(void)
{

}

void pmt_spi_begin_transaction(void)
{

}

void pmt_spi_end_transaction(void)
{

}

uint8_t pmt_spi_transfer(uint8_t data)
{
    unsigned char ret = 0;

    spi_tx_dma_dis(HSPI_MODULE);
    spi_rx_dma_dis(HSPI_MODULE);
    spi_tx_fifo_clr(HSPI_MODULE);
    spi_rx_fifo_clr(HSPI_MODULE);
    spi_tx_cnt(HSPI_MODULE, 1);
    spi_rx_cnt(HSPI_MODULE, 1);
    spi_set_transmode(HSPI_MODULE, SPI_MODE_WRITE_AND_READ);
    spi_set_cmd(HSPI_MODULE, 0x00);//when  cmd  disable that  will not sent cmd,just trigger spi send .
    spi_write(HSPI_MODULE,&data,1);
    spi_read(HSPI_MODULE,&ret,1);
    while (spi_is_busy(HSPI_MODULE));

    return ret;
}

void pmt_pin_write(int pin, bool state)
{
    gpio_set_level(pin, state);
}

void pmt_delay_us(uint32_t us)
{
    delay_us(us);
}

void pmt_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}

uint32_t pmt_get_millis(void)
{
    return get_system_time_ms();
}


void power_meter_app_init(void)
{
    delay_ms(100);
    uart_app_init();

    printf("power_meter_app_init \n");
    pmt_protocol_init();
    u8 rx_frame[5] = {0};

    gpio_function_en(PIN_SYN_);
    gpio_set_output(PIN_SYN_,1); 		//enable output
    gpio_set_up_down_res(PIN_SYN_,GPIO_PIN_PULLUP_10K);
    gpio_set_level(PIN_SYN_, 1);

    gpio_function_en(PIN_CS1);
    gpio_set_output(PIN_CS1,1); 		//enable output
    gpio_set_up_down_res(PIN_CS1,GPIO_PIN_PULLUP_10K);
    gpio_set_level(PIN_CS1, 1);

    gpio_function_en(PIN_CS2);
    gpio_set_output(PIN_CS2,1); 		//enable output
    gpio_set_up_down_res(PIN_CS2,GPIO_PIN_PULLUP_10K);
    gpio_set_level(PIN_CS2, 1);

    gpio_function_en(PIN_CS3);
    gpio_set_output(PIN_CS3,1); 		//enable output
    gpio_set_up_down_res(PIN_CS3,GPIO_PIN_PULLUP_10K);
    gpio_set_level(PIN_CS3, 1);

//	gpio_function_en(PIN_EN);
//	gpio_set_output(PIN_EN,1); 		//enable output
//	gpio_set_up_down_res(PIN_EN,GPIO_PIN_PULLUP_10K);
//	gpio_set_level(PIN_EN, 1);
//
//	delay_ms(50);
//	gpio_set_level(PIN_EN, 1);
//	delay_ms(50);


    pmt_handle[0] = stpm_create(PIN_EN, PIN_CS1, PIN_SYN_, 60);
    pmt_handle[1] = stpm_create(PIN_EN, PIN_CS2, PIN_SYN_, 60);
    pmt_handle[2] = stpm_create(PIN_EN, PIN_CS3, PIN_SYN_, 60);

    uint32_t vol, cur, acti, reacti;

    for(int i = 0; i < NUMBER_CHANNEL_POWERMETTER; i++)
    {



        pmt_handle[i]->spi_begin = pmt_spi_begin;
        pmt_handle[i]->spi_end = pmt_spi_end;
        pmt_handle[i]->spi_begin_transaction = pmt_spi_begin_transaction;
        pmt_handle[i]->spi_end_transaction = pmt_spi_end_transaction;
        pmt_handle[i]->spi_transfer = pmt_spi_transfer;

        pmt_handle[i]->auto_latch = false;
        pmt_handle[i]->pin_write = pmt_pin_write;
        pmt_handle[i]->delay_us = pmt_delay_us;
        pmt_handle[i]->delay_ms = pmt_delay_ms;
        pmt_handle[i]->get_millis = pmt_get_millis;

        pmt_handle[i]->context = &pmt_ctx[i];

        pmt_ctx[i].start_time = pmt_get_millis();
        pmt_ctx[i].last_update_time = pmt_ctx[i].start_time;
        pmt_ctx[i].id = i;


        if (!stpm_init(pmt_handle[i])) {
            printf("init err\n");
        }

        printf("init 0ke \n");
//		stpm_reset_energies(handle[0]);

    }

    read_calib();


}

void power_meter_app_loop(void)
{
    uart_print_app();
    uart_app_loop();
    pmt_protocol_loop();
    static uint64_t appTimeTick = 0;
    if(get_system_time_us() - appTimeTick > 10000){
        appTimeTick = get_system_time_us()  ; //1ms
    }
    else{
        return ;
    }

    stpm_monitoring_loop(pmt_handle);

}
uint32_t time_max = 0;
void read_stpm_data(stpm_handle_t *handle) {
    if (handle == NULL) return;
    pmt_data_context_t *ctx = handle->context;

    float voltage, current, reactive, apparent, fundamental;
    double active;

    stpm_read_rms_voltage_and_current(handle, 1, &voltage, &current );
    active = stpm_read_active_power(handle, 1);
    
    if( ctx->measurement.time > 10 )
    {
        ctx->accumulator.sample_count++;
    }

    if( ctx->measurement.time > time_max )
	{
    	time_max = ctx->measurement.time;
	}

    ctx->measurement.voltage += 3*0.1*(voltage - ctx->measurement.voltage );
	ctx->measurement.current += 3*0.1*(current - ctx->measurement.current );
	ctx->measurement.active_power += 3*0.1*(active - ctx->measurement.active_power );
    ctx->measurement.active_energy += (ctx->measurement.active_power * ctx->measurement.time) /3600000;
}

void update_energy(stpm_handle_t *handle) {
    if (handle == NULL) return;
    pmt_data_context_t *ctx = handle->context;

    stpm_update_energy(handle, 1);

    ctx->measurement.active_energy = stpm_read_active_energy(handle, 1);
    ctx->measurement.reactive_energy = stpm_read_reactive_energy(handle, 1);
}

void update_display(stpm_handle_t *handle, uint32_t current_time) {
    if (handle == NULL) return;
    pmt_data_context_t *ctx = handle->context;
//    update_energy(handle);

    G_POWER_METER.data.voltage = ctx->measurement.voltage;
    G_POWER_METER.data.frequency = (int)stpm_read_period(handle, 1);
    switch (ctx->id) {
    	case 0:
    		G_POWER_METER.data.current1 = ctx->measurement.current;
    		G_POWER_METER.data.power1 = ctx->measurement.active_power;
    		G_POWER_METER.data.energy1 = ctx->measurement.active_energy;
    		G_POWER_METER.data.time1 = current_time - ctx->start_time;
    		break;

        case 1:
    		G_POWER_METER.data.current2 = ctx->measurement.current;
    		G_POWER_METER.data.power2 = ctx->measurement.active_power;
    		G_POWER_METER.data.energy2 = ctx->measurement.active_energy;
    		G_POWER_METER.data.time2 = current_time - ctx->start_time;
    		break;

        case 2:
    		G_POWER_METER.data.current3 = ctx->measurement.current;
    		G_POWER_METER.data.power3 = ctx->measurement.active_power;
    		G_POWER_METER.data.energy3 = ctx->measurement.active_energy;
    		G_POWER_METER.data.time3 = current_time - ctx->start_time;
    		break;

    	default:
    		break;
    }

}

void stpm_monitoring_loop(stpm_handle_t **handle) {
    if (handle == NULL) return;

    uint32_t current_time = pmt_get_millis();
    stpm_latch_reg(handle[0]);
    for(uint8_t num_ch = 0; num_ch < NUMBER_CHANNEL_POWERMETTER; num_ch++)
    {
        pmt_data_context_t *ctx = handle[num_ch]->context;
        ctx->measurement.time = current_time -ctx->measurement.timestamp;
        ctx->measurement.timestamp = current_time; 
        read_stpm_data(handle[num_ch]);
        if (current_time - ctx->last_update_time >= 5000) {
            update_display(handle[num_ch], current_time);
            ctx->last_update_time = current_time;
        }
    }

}

void pmt_setcalib(uint8_t ch, float calib_U, float calib_I, float calib_P)
{
    pmt_handle[ch - 1]->calibration[1][0] = calib_U;
    pmt_handle[ch - 1]->calibration[1][1] = calib_I;
    pmt_handle[ch - 1]->calibration[1][2] = calib_P;
    save_calib();
}

void pmt_getcalib(uint8_t ch, float *calib_U, float *calib_I, float *calib_P)
{
    read_calib();
    *calib_U = pmt_handle[ch - 1]->calibration[1][0];
    *calib_I = pmt_handle[ch - 1]->calibration[1][1];
    *calib_P = pmt_handle[ch - 1]->calibration[1][2];
}

void pmt_setcalibr(uint8_t ch, uint16_t calib_U, uint16_t calib_I, uint16_t calib_P)
{
    stpm_handle_t *handle = pmt_handle[ch -1 ];
    stpm_update_calib(handle, 1, calib_U, calib_I);
}

void pmt_getcalibr(uint8_t ch, float *calib_U, float *calib_I, float *calib_P)
{
    stpm_handle_t *handle = pmt_handle[ch -1 ];
    DSP_CR5_bits_t dsp_cr5;
    DSP_CR6_bits_t dsp_cr6;

    stpm32_read_frame(handle, DSP_CR5_Address, handle->read_buffer);
    dsp_cr5.LSW.LSB = handle->read_buffer[0];
    dsp_cr5.LSW.MSB = handle->read_buffer[1];
    dsp_cr5.MSW.LSB = handle->read_buffer[2];
    dsp_cr5.MSW.MSB = handle->read_buffer[3];
    printf("dsp_cr5: %08x\n", dsp_cr5);

    stpm32_read_frame(handle, DSP_CR6_Address, handle->read_buffer);
    dsp_cr6.LSW.LSB = handle->read_buffer[0];
    dsp_cr6.LSW.MSB = handle->read_buffer[1];
    dsp_cr6.MSW.LSB = handle->read_buffer[2];
    dsp_cr6.MSW.MSB = handle->read_buffer[3];
    printf("dsp_cr6: %08x\n", dsp_cr6);

    *calib_U = dsp_cr5.CHV1;
    *calib_I = dsp_cr6.CHC1;
    *calib_P = 0;
}

float pmt_read_U(uint8_t ch)
{
    pmt_data_context_t *ctx = pmt_handle[ch -1]->context;
    return ctx->measurement.voltage;
}

float pmt_read_I(uint8_t ch)
{
    pmt_data_context_t *ctx = pmt_handle[ch -1]->context;
    return ctx->measurement.current;
}

float pmt_read_P(uint8_t ch)
{
    pmt_data_context_t *ctx = pmt_handle[ch -1]->context;
    return ctx->measurement.active_power;
}

void pmt_print_info(uint8_t ch)
{
    stpm_handle_t *handle = pmt_handle[ch -1 ];
    pmt_data_context_t *ctx = handle->context;
    
    uint32_t current_time = pmt_get_millis();

    uint32_t elapsed_seconds = (current_time - ctx->start_time) / 1000;
    uint32_t hours = elapsed_seconds / 3600;
    uint32_t minutes = (elapsed_seconds % 3600) / 60;
    uint32_t seconds = elapsed_seconds % 60;
    uint8_t buff[5];
    uint32_t fre_ch1, fre_ch2;
    stpm_read_periods(handle, & fre_ch1, fre_ch2);
    // stpm32_read_frame(handle, PH1_Active_Power_Address, buff);
	// printf("%02x%02x%02x%02x\n", buff[0], buff[1], buff[2], buff[3] );
    // printf("%10.3f\n", stpm_read_active_power(handle, 1));
    printf("========================================\n");
    printf("STPM32 ID: %d - Measurement Data\n", index);
    printf("freqecen: %10.3f \n", stpm_read_period(handle, 1));
    // printf("========================================\n");
    printf("timetamp: %02u:%02u:%02u\n", hours, minutes, seconds);
    printf("number of reasd: %d\n", ctx->accumulator.sample_count);
    printf("deta time max: %u\n", time_max);
    printf("voltaget rsm (V):   %10.3f V\n", ctx->measurement.voltage);
    printf("current  rsm (A):   %10.3f A\n", ctx->measurement.current);
    printf("active_power:       %10.3f W\n", ctx->measurement.active_power);
    printf("active_energy:      %10.3f Wh\n", ctx->measurement.active_energy);
    printf("========================================\n");

    time_max = 0;
}

static void save_calib()
{
    u8 tbs_profile[36] = {0};

    for (int i = 0; i < NUMBER_CHANNEL_POWERMETTER; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            float f = pmt_handle[i]->calibration[1][j];
            int base = j * 4 + i * 12;

            memcpy(&tbs_profile[base], &f, sizeof(float));
        }
    }

    fl_db_tbsprofile_save((u8*)tbs_profile, sizeof(tbs_profile));
}

static void read_calib()
{
    fl_tbs_data_t tbs_load = fl_db_tbsprofile_load(); // 36 bytes

    for (int i = 0; i < NUMBER_CHANNEL_POWERMETTER; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            int base = j * 4 + i * 12;

            float f;
            memcpy(&f, &tbs_load.data[base], sizeof(float));

            pmt_handle[i]->calibration[1][j] = f;
        }
    }
}


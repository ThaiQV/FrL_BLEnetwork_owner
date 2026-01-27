/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: pmt_app.c
 *Created on		: Jan 24, 2026
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "tl_common.h"
#ifdef POWER_METER_DEVICE
#include "vendor/FrL_network/fl_nwk_handler.h"
#include "fl_driver/fl_stpm32/stpm32.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define PMT_LOG_DEBUG			1

#define PMT_DEBUG()				{PLOG_Start(PERI);PLOG_Start(USER);}
#define STPM32_1_EN_PIN			GPIO_PD3
#define STPM32_2_EN_PIN			GPIO_PD3
#define STPM32_3_EN_PIN			GPIO_PD3

#define STPM32_1_CS_PIN			HSPI_CS_POWER_METER_STPM1
#define STPM32_2_CS_PIN			HSPI_CS_POWER_METER_STPM2
#define STPM32_3_CS_PIN			HSPI_CS_POWER_METER_STPM3

#define STPM32_HSPI_CLK			HSPI_CLK
#define STPM32_HSPI_MOSI		HSPI_MOSI
#define STPM32_HSPI_MISO		HSPI_MISO
#define STPM32_HSPI_SYNC		HSPI_IO

#define STPM32_FREQ_NET			60 //parameter input to stpm32 lib

#define POWERMETER_CHANNEL		3

static stpm_handle_t *PMT_STRUCT[POWERMETER_CHANNEL];
//static pmt_data_context_t PMT_CTX[POWERMETER_CHANNEL];
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

static void pmt_calip(void *_arg, u8 _size) {
    double *args = (double *)_arg;  //
    LOGA(USER,"Calip cmd(%d): %.2lf,%.2lf,%.2lf,%.2lf,%.2lf\r\n",_size,
         (_size > 0 ? args[0] : 0.0),
         (_size > 1 ? args[1] : 0.0),
         (_size > 2 ? args[2] : 0.0),
         (_size > 3 ? args[3] : 0.0),
         (_size > 4 ? args[4] : 0.0));

    if(!_size){
    	float period[POWERMETER_CHANNEL];
    	period[0] = stpm_read_period(PMT_STRUCT[0],1);
    	period[1] = stpm_read_period(PMT_STRUCT[1],1);
    	period[2] = stpm_read_period(PMT_STRUCT[2],1);
    	LOGA(PERI,"f:%.f,%.f,%.f\r\n",period[0],period[1],period[2]);
    	LOGA(PERI,"%f\r\n",stpm_read_voltage(PMT_STRUCT[2],1));
    }
}
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

uint8_t stpm32_spi_transfer(uint8_t data)
{
	unsigned char ret = 0;

	// spi_tx_dma_dis(HSPI_MODULE);
	// spi_rx_dma_dis(HSPI_MODULE);
	spi_tx_fifo_clr(HSPI_MODULE);
	spi_rx_fifo_clr(HSPI_MODULE);
	spi_tx_cnt(HSPI_MODULE,1);
	spi_rx_cnt(HSPI_MODULE,1);
	// spi_set_irq_mask(HSPI_MODULE, SPI_RXFIFO_OR_INT_EN);
	spi_set_transmode(HSPI_MODULE,SPI_MODE_WRITE_READ);
	spi_set_cmd(HSPI_MODULE,0x00); // when  cmd  disable that  will not sent cmd,just trigger spi send .
	spi_write(HSPI_MODULE,&data,1);
	spi_read(HSPI_MODULE,&ret,1);
	while (spi_is_busy(HSPI_MODULE))
		;
	// spi_clr_irq_mask(HSPI_MODULE, SPI_RXFIFO_OR_INT_EN);
	return ret;
}

void stpm32_spi_transfer_buffer(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len) {
	if (len == 0)
		return;
	// Clear FIFO
	spi_tx_fifo_clr(HSPI_MODULE);
	spi_rx_fifo_clr(HSPI_MODULE);

	// Set count
	spi_tx_cnt(HSPI_MODULE,len);
	spi_rx_cnt(HSPI_MODULE,len);
	// Set mode
	spi_set_transmode(HSPI_MODULE,SPI_MODE_WRITE_AND_READ);
	spi_set_cmd(HSPI_MODULE,0x00);
	// Write all bytes
	spi_write(HSPI_MODULE,tx_buf,len);
	// Read all bytes
	spi_read(HSPI_MODULE,rx_buf,len);
	while (spi_is_busy(HSPI_MODULE))
		;
}

uint32_t pmt_get_millis(void){
	 return clock_time()/ SYSTEM_TIMER_TICK_1MS;
}

void pmt_spi_begin(void){}
void pmt_spi_end(void){}
void pmt_spi_begin_transaction(void){}
void pmt_spi_end_transaction(void){}
void pmt_pin_write(int pin, bool state){gpio_set_level(pin, state);}
void pmt_delay_us(uint32_t us){delay_us(us);}
void pmt_delay_ms(uint32_t ms){delay_ms(ms);}

void pmt_init(void){
	PMT_DEBUG();

//    gpio_function_en(STPM32_HSPI_SYNC);
//    gpio_set_output(STPM32_HSPI_SYNC, 1); // enable output
//    gpio_set_up_down_res(STPM32_HSPI_SYNC, GPIO_PIN_PULLUP_10K);
//    gpio_set_level(STPM32_HSPI_SYNC, 1);
//
    gpio_function_en(STPM32_1_CS_PIN);
    gpio_set_output(STPM32_1_CS_PIN, 1); // enable output
    gpio_set_up_down_res(STPM32_1_CS_PIN, GPIO_PIN_PULLUP_10K);
    gpio_set_level(STPM32_1_CS_PIN, 1);

    gpio_function_en(STPM32_2_CS_PIN);
    gpio_set_output(STPM32_2_CS_PIN, 1); // enable output
    gpio_set_up_down_res(STPM32_2_CS_PIN, GPIO_PIN_PULLUP_10K);
    gpio_set_level(STPM32_2_CS_PIN, 1);

    gpio_function_en(STPM32_3_CS_PIN);
    gpio_set_output(STPM32_3_CS_PIN, 1); // enable output
    gpio_set_up_down_res(STPM32_3_CS_PIN, GPIO_PIN_PULLUP_10K);
    gpio_set_level(STPM32_3_CS_PIN, 1);

//	stpm_handle_t *tmp = stpm_create( STPM32_1_EN_PIN,  STPM32_1_CS_PIN,  STPM32_HSPI_SYNC,  STPM32_FREQ_NET);
//	PMT_STRUCT[0] = *tmp;
//	stpm_destroy(tmp);
//	tmp=stpm_create( STPM32_2_EN_PIN,  STPM32_2_CS_PIN,  STPM32_HSPI_SYNC,  STPM32_FREQ_NET);
//	PMT_STRUCT[1] = *tmp;
//	stpm_destroy(tmp);
//	tmp=stpm_create( STPM32_3_EN_PIN,  STPM32_3_CS_PIN,  STPM32_HSPI_SYNC,  STPM32_FREQ_NET);
//	PMT_STRUCT[2] = *tmp;
//	stpm_destroy(tmp);

	PMT_STRUCT[0] =stpm_create( STPM32_1_EN_PIN,  STPM32_1_CS_PIN,  STPM32_HSPI_SYNC,  STPM32_FREQ_NET);
	PMT_STRUCT[1] =stpm_create( STPM32_2_EN_PIN,  STPM32_2_CS_PIN,  STPM32_HSPI_SYNC,  STPM32_FREQ_NET);
	PMT_STRUCT[2] =stpm_create( STPM32_3_EN_PIN,  STPM32_3_CS_PIN,  STPM32_HSPI_SYNC,  STPM32_FREQ_NET);
	//spi init
    for (u8 i = 0; i < POWERMETER_CHANNEL; i++)
    {
        PMT_STRUCT[i]->spi_begin = pmt_spi_begin;
        PMT_STRUCT[i]->spi_end = pmt_spi_end;
        PMT_STRUCT[i]->spi_begin_transaction = pmt_spi_begin_transaction;
        PMT_STRUCT[i]->spi_end_transaction = pmt_spi_end_transaction;
        PMT_STRUCT[i]->spi_transfer = stpm32_spi_transfer;
        PMT_STRUCT[i]->spi_transfer_buffer = stpm32_spi_transfer_buffer;

        PMT_STRUCT[i]->auto_latch = true;
        PMT_STRUCT[i]->pin_write = pmt_pin_write;
        PMT_STRUCT[i]->delay_us = pmt_delay_us;
        PMT_STRUCT[i]->delay_ms = pmt_delay_ms;
        PMT_STRUCT[i]->get_millis = pmt_get_millis;
//
//        PMT_STRUCT[i]->context = &PMT_CTX[i];
//
//        PMT_CTX[i].start_time = pmt_get_millis();
//        PMT_CTX[i].last_update_time = PMT_CTX[i].start_time;
//        PMT_CTX[i].id = i;

        if (!stpm_init(PMT_STRUCT[i]))
        {
        	ERR(PERI,"STPM32-%d Init err\n",i+1);
        }
        LOGA(PERI,"STPM32-%d Init ok!\n",i+1);
        stpm_update_calib(PMT_STRUCT[i],1, 1, 1);
        stpm_set_reference_frequency(PMT_STRUCT[i],STPM3X_FREQ_60HZ);
        //		stpm_reset_energies(handle[0]);
        LOGA(PERI,"[%d]volt:%f,freq:%f\r\n",i,stpm_read_voltage(PMT_STRUCT[0],1),stpm_read_period(PMT_STRUCT[0],1));
    }
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

typedef struct{
	char* cmd_name;
	void (*ExcFunc)(void*,u8);
}pmt_cmd_t;

pmt_cmd_t PMT_CMD[] = {{"calip",pmt_calip}};

void pmt_serial_proc(u8* _cmd,u8 _len){
	char cmd[10];
	double para[5];
//	u8 arg[200];
//	memset(arg,0,SIZEU8(arg));
	int rslt = sscanf((char*) _cmd,"%s %lf %lf %lf %lf %lf",cmd,&para[0],&para[1],&para[2],&para[3],&para[4]);
	if (rslt >= 1) {
		for (u8 var = 0; var < sizeof(PMT_CMD)/sizeof(PMT_CMD[0]); ++var) {
			if(-1 != plog_IndexOf((u8*)cmd,(u8*)PMT_CMD[var].cmd_name,strlen(PMT_CMD[var].cmd_name),strlen(cmd))){
				PMT_CMD[var].ExcFunc((void*)para,rslt-1);
			}
		}
	}
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/


#endif //POWER_METER_DEVICE

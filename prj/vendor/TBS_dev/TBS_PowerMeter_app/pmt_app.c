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
#define PMT_LOG_DEBUG				1

#define PMT_DEBUG()					{PLOG_Start(PERI);PLOG_Start(USER);}
#define STPM32_1_EN_PIN				GPIO_PD3 //not use
#define STPM32_2_EN_PIN				STPM32_1_EN_PIN
#define STPM32_3_EN_PIN				STPM32_1_EN_PIN

#define STPM32_1_CS_PIN				HSPI_CS_POWER_METER_STPM1
#define STPM32_2_CS_PIN				HSPI_CS_POWER_METER_STPM2
#define STPM32_3_CS_PIN				HSPI_CS_POWER_METER_STPM3
#define STPM32_HSPI_SYNC			HSPI_SYNC

#define STPM32_HSPI_CLK				HSPI_CLK
#define STPM32_HSPI_MOSI			HSPI_MOSI
#define STPM32_HSPI_MISO			HSPI_MISO

#define STPM32_SPI_PIN_INIT(pin)	PERI_SPI_PIN_INIT(pin)

#define STPM32_FREQ_NET				STPM3X_FREQ_60HZ //parameter input to stpm32 lib

#define POWERMETER_CHANNEL			3

static stpm_handle_t *PMT_STRUCT[POWERMETER_CHANNEL];
//static pmt_data_context_t PMT_CTX[POWERMETER_CHANNEL];

#define PMT_GET_CALIB_V(chnX)			(PMT_STRUCT[chnX]->calibration[1][0])
#define PMT_GET_CALIB_C(chnX)			(PMT_STRUCT[chnX]->calibration[1][1])
#define PMT_GET_CALIB_E(chnX)			(PMT_STRUCT[chnX]->calibration[1][2])

#define PMT_SET_CALIB_V(chnX,valueV)	(PMT_STRUCT[chnX]->calibration[1][0] = (float)valueV)
#define PMT_SET_CALIB_C(chnX,valueC)	(PMT_STRUCT[chnX]->calibration[1][1] = (float)valueC)
#define PMT_SET_CALIB_E(chnX,valueE)	(PMT_STRUCT[chnX]->calibration[1][2] = (float)valueE)

#define PMT_CLACH_ALL()					{stpm_latch_reg(PMT_STRUCT[0]);}
#define PMT_CHECK_CHN(x)				{if(x>POWERMETER_CHANNEL)return;}


uint32_t pmt_get_millis(void);
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
s32 _interval_read_sensor(void){

	PMT_CLACH_ALL();//important

	float period[3], volt[3], curr[3];
	float power[3], energy[3];
	float P_factor[3];
	P_INFO("*********************************************************\r\n");
	for (u8 chn = 0; chn < POWERMETER_CHANNEL; ++chn) {
		period[chn] = stpm_read_period(PMT_STRUCT[chn],1);
		stpm_read_rms_voltage_and_current(PMT_STRUCT[chn],1,&volt[chn],&curr[chn]);
		power[chn] = 1000.0*stpm_read_active_power(PMT_STRUCT[chn],1);
		energy[chn] = stpm_read_active_energy(PMT_STRUCT[chn],1);
		P_factor[chn] = stpm_read_power_factor(PMT_STRUCT[chn],1);
		P_INFO("[%d]F:%.1f,Vrms:%.3f,Irms:%.3f,P:%.6f,E:%.3f,P(factor):%.3f\r\n",
				chn,period[chn],volt[chn],curr[chn],power[chn],energy[chn],P_factor[chn]);
	}
	P_INFO("*********************************************************\r\n");
	return 0;
}
s32 _auto_calibE(void){
	u32 delta_millis =0;
	float e_read=0;
	float e_last_read=0;
	float p_read=1;
	float calib_E=1;
	PMT_CLACH_ALL();//important
	for(u8 chn=0;chn<POWERMETER_CHANNEL;chn++){
		if(PMT_STRUCT[chn]->flag_calib){
			delta_millis = pmt_get_millis()- PMT_STRUCT[chn]->total_energy.valid_millis;
			e_last_read = PMT_STRUCT[chn]->total_energy.active;
			e_read = stpm_read_active_energy(PMT_STRUCT[chn],1);
			p_read = (e_read-e_last_read)*3600.0/(delta_millis*0.001);
			calib_E= PMT_STRUCT[chn]->flag_calib/p_read;
			PMT_SET_CALIB_E(chn,calib_E);
			P_INFO("Auto calib E(chn %d):Pload:%.2f,Pread:%.2f=>Calib_E:%.3f\r\n",chn+1,PMT_STRUCT[chn]->flag_calib,p_read,PMT_GET_CALIB_E(chn));
			PMT_STRUCT[chn]->flag_calib=0;
		}
	}
	return -1;
}
void pmt_calib_stpm(u8 _chn,bool _set,uint16_t _calib_V, uint16_t _calib_C,uint16_t _calip_E){

	PMT_CHECK_CHN(_chn);
    LOGA(USER,"[%d]Calib_V:%d,Calib_C:%d,Calib_E:%d\r\n",_chn,_calib_V,  _calib_C, _calip_E);

	if(!_set){
		u8 numofchn = _chn == 0 ? POWERMETER_CHANNEL : _chn;
		float calib_V,calib_C;
		u16 calib_V_stpm ;
		u16 calib_C_stpm ;
		for (u8 chn = (_chn>0?_chn-1:0); chn < numofchn; ++chn) {
			stpm_read_calib(PMT_STRUCT[chn],1,&calib_V_stpm,&calib_C_stpm);//hw calib in the address => not use
			calib_V = PMT_GET_CALIB_V(chn); //soft calib
			calib_C = PMT_GET_CALIB_C(chn); //soft calib
			P_INFO("[%d]Calib_U:%.3f/%d,Calib_I:%.3f/%d,Calib_P:%.3f\r\n",chn,calib_V,calib_V_stpm,calib_C,calib_C_stpm,PMT_GET_CALIB_E(chn));
		}
	}else
	{
		u8 chn_idx = _chn -1;
		float volt,curr;
		float backup_calibV = 1;
		float backup_calibC = 1;

		if(_calib_V || _calib_C){
			backup_calibV = PMT_GET_CALIB_V(chn_idx);
			backup_calibC = PMT_GET_CALIB_C(chn_idx);

			if(_calib_V) PMT_SET_CALIB_V(chn_idx,1); //set default
			if(_calib_C) PMT_SET_CALIB_C(chn_idx,1); //set default

			delay_ms(10); ///delay to stpm refesh new data
			stpm_read_rms_voltage_and_current(PMT_STRUCT[chn_idx],1,&volt,&curr);
			if(!volt || !curr){
				ERR(PERI,"NULL voltage or current sensors!!!!\r\n");
				return;
			}
			if(_calib_V) {
				backup_calibV = ((float) _calib_V / 100)/volt;
				//update calib's value
				P_INFO("[%d]Ureal:%.2f,Uread:%.2f=>calibU:%.2f\r\n",chn_idx,((float )_calib_V) / 100,volt,backup_calibV);
				PMT_SET_CALIB_V(chn_idx, backup_calibV);
			}
			if(_calib_C) {
				backup_calibC = ((float) _calib_C / 1000)/curr;
				//update calib's value
				P_INFO("[%d]Ireal:%.3f,Iread:%.3f=>calibI:%.3f\r\n",chn_idx,((float )_calib_C) / 1000,curr,backup_calibC);
				PMT_SET_CALIB_C(chn_idx, backup_calibC);
			}
			//calculate calibP
			//P_INFO("[%d]Preal:%.3f,Iread:%.3f=>calibI:%.3f\r\n",chn_idx,((float )_calib_C) / 1000,curr,backup_calibC);
			//PMT_SET_CALIB_P(chn_idx,backup_calibV*backup_calibC);
			if(_calip_E){
				blt_soft_timer_delete(_interval_read_sensor);
				//Start automatical calib E
				stpm_reset_energies(PMT_STRUCT[chn_idx]);
				PMT_STRUCT[chn_idx]->flag_calib=0.01*_calip_E;
				blt_soft_timer_restart(_auto_calibE,5*1000*1000); //5s
			}else{
				PMT_STRUCT[chn_idx]->flag_calib = 0;
				blt_soft_timer_delete(_auto_calibE);
			}
		}
	}
}


void pmt_info(void *_arg, u8 _size){
	double* data = (double*)_arg;

	if(_size > 0 && data[0] > 0){
		P_INFO("Start interval read sensors:%d ms\r\n",(u32)data[0]);
		blt_soft_timer_restart(_interval_read_sensor,data[0]*1000);
	}
	else{
		blt_soft_timer_delete(_interval_read_sensor);
	}
	PMT_CLACH_ALL();//important
	float period[3], volt[3], curr[3];
	float power[3], energy[3];
	float P_factor[3];
	P_INFO("*********************************************************\r\n");
	for (u8 chn = 0; chn < POWERMETER_CHANNEL; ++chn) {
		period[chn] = stpm_read_period(PMT_STRUCT[chn],1);
		stpm_read_rms_voltage_and_current(PMT_STRUCT[chn],1,&volt[chn],&curr[chn]);
		power[chn] = stpm_read_active_power(PMT_STRUCT[chn],1);
		energy[chn] = stpm_read_active_energy(PMT_STRUCT[chn],1);
		P_factor[chn] = stpm_read_power_factor(PMT_STRUCT[chn],1);
		P_INFO("[%d]F:%.1f,Vrms:%.3f,Irms:%.3f,P:%.6f,E:%.3f,P(factor):%.3f\r\n",
				chn,period[chn],volt[chn],curr[chn],power[chn],energy[chn],P_factor[chn]);
	}
	P_INFO("*********************************************************\r\n");
}
static void pmt_reset_energy(void *_arg, u8 _size) {
    u8 *args = (u8 *)_arg;  //
    memset(args+_size,0,6);
	u8 numofchn = args[0] == 0 ? POWERMETER_CHANNEL : args[0];
	PMT_CLACH_ALL();
	for (u8 chn = (args[0] > 0 ? args[0] - 1 : 0); chn < numofchn; ++chn) {
		LOGA(USER,"[%d]Reset Energy:%.3f\r\n",chn);
		stpm_reset_energies(PMT_STRUCT[chn]);
	}
}

static void pmt_calib(void *_arg, u8 _size) {
    double *args = (double *)_arg;  //
    memset(args+_size,0,6);
    LOGA(USER,"Calib cmd(%d):%.3lf, %.3lf,%.3lf,%.3lf,%.3lf,%.3lf\r\n",_size,args[0],args[1],args[2],args[3],args[4],args[5]);

    if(!_size){
    	pmt_calib_stpm(0,0,0,0,0); //get
    }
    else {
		PMT_CLACH_ALL();//important
    	pmt_calib_stpm(1,1,100*args[0],1000*args[1],100*args[2]);
    	pmt_calib_stpm(2,1,100*args[0],1000*args[1],100*args[2]);
    	pmt_calib_stpm(3,1,100*args[0],1000*args[1],100*args[2]);
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

//	spi_tx_dma_dis(HSPI_MODULE);
//	spi_rx_dma_dis(HSPI_MODULE);
	spi_tx_fifo_clr(HSPI_MODULE);
	spi_rx_fifo_clr(HSPI_MODULE);
	spi_tx_cnt(HSPI_MODULE,1);
	spi_rx_cnt(HSPI_MODULE,1);
	// spi_set_irq_mask(HSPI_MODULE, SPI_RXFIFO_OR_INT_EN);
	spi_set_transmode(HSPI_MODULE,SPI_MODE_WRITE_AND_READ);
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
//	spi_tx_dma_dis(HSPI_MODULE);
//	spi_rx_dma_dis(HSPI_MODULE);
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

	STPM32_SPI_PIN_INIT(STPM32_1_CS_PIN);
	STPM32_SPI_PIN_INIT(STPM32_2_CS_PIN);
	STPM32_SPI_PIN_INIT(STPM32_3_CS_PIN);

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
        //add Ki,Ku,Kp
        PMT_SET_CALIB_V(i,1);
        PMT_SET_CALIB_C(i,1);
        PMT_SET_CALIB_E(i,1);
    }

    //Settings chip
    PMT_CLACH_ALL();
    stpm_set_current_gain(PMT_STRUCT[0],1,STPM3X_GAIN_2X);
    stpm_set_current_gain(PMT_STRUCT[1],1,STPM3X_GAIN_2X);
    stpm_set_current_gain(PMT_STRUCT[2],1,STPM3X_GAIN_2X);
    //reset energy
    stpm_reset_energies(PMT_STRUCT[0]);
    stpm_reset_energies(PMT_STRUCT[1]);
    stpm_reset_energies(PMT_STRUCT[2]);
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

pmt_cmd_t PMT_CMD[] = {
					{"calib",pmt_calib},
					{"readinfo",pmt_info},
					{"resetenergy",pmt_reset_energy},};

void pmt_serial_proc(u8* _cmd,u8 _len){
	char cmd[20];
	double para[6]={0,0,0,0,0,0};
//	u8 arg[200];
//	memset(arg,0,SIZEU8(arg));
	int rslt = sscanf((char*) _cmd,"%s %lf %lf %lf %lf %lf %lf",cmd,&para[0],&para[1],&para[2],&para[3],&para[4],&para[5]);
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

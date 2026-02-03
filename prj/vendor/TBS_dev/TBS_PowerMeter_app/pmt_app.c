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
#include "math.h"
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define PMT_LOG_DEBUG				0
#if PMT_LOG_DEBUG
#define PMT_DEBUG()					{PLOG_Start(PERI);PLOG_Start(USER);}
#else
#define PMT_DEBUG()
#endif
#define PMT_DEBUG_STOP()			{PLOG_Stop(PERI);PLOG_Stop(USER);}

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

#define STPM32_FREQ_NET				STPM3X_FREQ_50HZ //parameter input to stpm32 lib
#define STPM32_GAIN					STPM3X_GAIN_2X

#define POWERMETER_CHANNEL			3

typedef struct
{
	float e_last_stored;
	float e_sum;
	float p_avg;
	u32 working_time_ms;
	float i_avg;
	float u_avg;
	u32 numofcount_avg;
} pmt_data_context_t;

static stpm_handle_t *PMT_STRUCT[POWERMETER_CHANNEL];
static pmt_data_context_t PMT_CTX[POWERMETER_CHANNEL];
extern tbs_device_powermeter_t G_POWER_METER ;

#define PMT_GET_CALIB_V(chnX)			(PMT_STRUCT[chnX]->calibration[1][0])
#define PMT_GET_CALIB_C(chnX)			(PMT_STRUCT[chnX]->calibration[1][1])
#define PMT_GET_CALIB_E(chnX)			(PMT_STRUCT[chnX]->calibration[1][2])

#define PMT_SET_CALIB_V(chnX,valueV)	(PMT_STRUCT[chnX]->calibration[1][0] = (float)valueV)
#define PMT_SET_CALIB_C(chnX,valueC)	(PMT_STRUCT[chnX]->calibration[1][1] = (float)valueC)
#define PMT_SET_CALIB_E(chnX,valueE)	(PMT_STRUCT[chnX]->calibration[1][2] = (float)valueE)

#define PMT_CALIB_RUNNING()				(PMT_STRUCT[0]->flag_calib || PMT_STRUCT[1]->flag_calib || PMT_STRUCT[1]->flag_calib)

#define PMT_CHECK_CHN(x)				{if(x>POWERMETER_CHANNEL)return;}

#define PMT_SAMPLE_TIMING				300//ms

extern u16 G_POWER_METER_PARAMETER[4];

#define PMT_GET_THRESHOLD(chnX)			(u16)(G_POWER_METER_PARAMETER[chnX])

#define PMT_PF_AVG(chnX)				(((chnX==1)?G_POWER_METER.data.fac_power1:(chnX==2)?G_POWER_METER.data.fac_power2:G_POWER_METER.data.fac_power3)&0x7F)
uint32_t pmt_get_millis(void);
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

void PMT_LATCH_ALL(void){
	static u32 last_latch=0;
	if(clock_time_exceed(last_latch,100*1000)){
		stpm_latch_reg(PMT_STRUCT[0]);
		last_latch=clock_time();
	}else{
		//ERR(PERI,"Latch already....\r\n");
	}
}

static void _pmt_save_calib(void)
{
    u8 tbs_profile[64] = {0};
    fl_tbs_data_t tbs_load = fl_db_tbsprofile_load();
    memcpy(tbs_profile, tbs_load.data, SIZEU8(tbs_profile));

    for (int i = 0; i < POWERMETER_CHANNEL; i++)
    {
        for (int j = 0; j < POWERMETER_CHANNEL; j++)
        {
            float f = PMT_STRUCT[i]->calibration[1][j];
            int base = j * 4 + i * 12;
            memcpy(&tbs_profile[base], &f, sizeof(float));
        }
    }
    fl_db_tbsprofile_save((u8*)tbs_profile, sizeof(tbs_profile));
}

static void _pmt_load_calib(void)
{
   fl_tbs_data_t tbs_load = fl_db_tbsprofile_load();

   for (int i = 0; i < POWERMETER_CHANNEL; i++)
   {
       for (int j = 0; j < 3; j++)
       {
           int base = j * 4 + i * 12;

           float f;
           memcpy(&f, &tbs_load.data[base], sizeof(float));

           PMT_STRUCT[i]->calibration[1][j] = f;
       }
   }
}
//
static void _pmt_load_context(void) {
	fl_tbs_data_t tbs_load = fl_db_tbsprofile_load();
	u8 slot_memory=SIZEU8(PMT_STRUCT[0]->calibration);
	float f;
	u32 wt;
	for (int i = 0; i < POWERMETER_CHANNEL; i++) {
		memcpy(&f,&tbs_load.data[slot_memory],sizeof(PMT_CTX[i].e_sum));
		//PMT_CTX[i].e_sum=f;
		PMT_CTX[i].e_last_stored =f;
		memcpy(&wt,&tbs_load.data[slot_memory+sizeof(PMT_CTX[i].e_sum)],sizeof(PMT_CTX[i].working_time_ms));
		PMT_CTX[i].working_time_ms=wt;
		slot_memory+=sizeof(PMT_CTX[i].e_sum)+sizeof(PMT_CTX[i].working_time_ms);
	}
}

static void _pmt_save_context(void)
{
	u8 tbs_profile[128] = { 0 };
	fl_tbs_data_t tbs_load = fl_db_tbsprofile_load();
	memcpy(tbs_profile,tbs_load.data,SIZEU8(tbs_profile));
	u8 slot_memory=SIZEU8(PMT_STRUCT[0]->calibration);
	for (int i = 0; i < POWERMETER_CHANNEL; i++) {
		memcpy(&tbs_profile[slot_memory],&PMT_CTX[i].e_sum,sizeof(PMT_CTX[i].e_sum));
		memcpy(&tbs_profile[slot_memory+sizeof(PMT_CTX[i].e_sum)],&PMT_CTX[i].working_time_ms,sizeof(PMT_CTX[i].working_time_ms));
		slot_memory+=sizeof(PMT_CTX[i].e_sum)+sizeof(PMT_CTX[i].working_time_ms);
	}
	fl_db_tbsprofile_save((u8*) tbs_profile,sizeof(tbs_profile));
}
s32 _interval_read_sensor(void){
	//cancel if automatical calibE running
	if (PMT_STRUCT[0]->flag_calib || PMT_STRUCT[1]->flag_calib || PMT_STRUCT[2]->flag_calib) {
		ERR(PERI,"Automatic CalibE running.....\r\n");
		return 0;
	}
	PMT_LATCH_ALL();//important

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
		P_INFO("[%d]F:%.1f,Vrms:%.3f,Irms:%.3f,P:%.6f,E:%.6f,P(factor):%.3f\r\n",
				chn,period[chn],volt[chn],curr[chn],power[chn],energy[chn],P_factor[chn]);
	}
	P_INFO("*********************************************************\r\n");
	return 0;
}
s32 _auto_calibE_Exc(void) {
	u32 delta_millis = 0;
	float e_read = 0;
	//float e_last_read=0;
	float e_cal = 0;
//	float p_read=1;
	float calib_E = 1;

	if (PMT_CALIB_RUNNING()) {
		PMT_LATCH_ALL(); //important
		for (u8 chn = 0; chn < POWERMETER_CHANNEL; chn++) {
			if (PMT_STRUCT[chn]->flag_calib) {
				delta_millis = pmt_get_millis() - PMT_STRUCT[chn]->total_energy.valid_millis;
				e_cal = (((1000.0 * stpm_read_active_power(PMT_STRUCT[chn],1) + PMT_STRUCT[chn]->flag_calib) / 2) * delta_millis * 0.001) / 3600.0; //Wh
				e_cal = 0.001 * e_cal; // =>Wh->kWh
				e_read = stpm_read_active_energy(PMT_STRUCT[chn],1);
				calib_E = e_cal / e_read;
				PMT_SET_CALIB_E(chn,calib_E);
				P_INFO("Auto calib E(chn %d):delta_t:%d,Ecal:%.6f,Eread:%.6f=>Calib_E:%.6f\r\n",chn + 1,delta_millis,e_cal,e_read,
						PMT_GET_CALIB_E(chn));
				PMT_STRUCT[chn]->flag_calib = 0;
			}
		}
		_pmt_save_calib();
	}
	return -1;
}
s32 _auto_calibE_collectData(void) {
	if (PMT_CALIB_RUNNING()) {
		PMT_LATCH_ALL(); //important
		for (u8 chn = 0; chn < POWERMETER_CHANNEL; chn++) {
			if (PMT_STRUCT[chn]->flag_calib) {
				PMT_STRUCT[chn]->flag_calib = 1000.0 * stpm_read_active_power(PMT_STRUCT[chn],1);
				stpm_reset_energies(PMT_STRUCT[chn]);
				blt_soft_timer_restart(_auto_calibE_Exc,10 * 1000 * 1000); //1s
			}
		}
	}
	return -1;
}

void pmt_calib_stpm(u8 _chn,bool _set,uint16_t _calib_V, uint16_t _calib_C,uint16_t _calip_E){

	PMT_CHECK_CHN(_chn);
    LOGA(USER,"[%d]Calib_V:%d,Calib_C:%d,Calib_E:%d\r\n",_chn,_calib_V,  _calib_C, _calip_E);

	if(!_set){
		_pmt_load_calib();
		u8 numofchn = _chn == 0 ? POWERMETER_CHANNEL : _chn;
		float calib_V,calib_C;
		u16 calib_V_stpm ;
		u16 calib_C_stpm ;
		for (u8 chn = (_chn>0?_chn-1:0); chn < numofchn; ++chn) {
			stpm_read_calib(PMT_STRUCT[chn],1,&calib_V_stpm,&calib_C_stpm);//hw calib in the address => not use
			calib_V = PMT_GET_CALIB_V(chn); //soft calib
			calib_C = PMT_GET_CALIB_C(chn); //soft calib
			P_INFO("[%d]Calib_U:%.3f/%d,Calib_I:%.3f/%d,Calib_E:%.6f\r\n",chn,calib_V,calib_V_stpm,calib_C,calib_C_stpm,PMT_GET_CALIB_E(chn));
		}
	}else
	{
		u8 chn_idx = _chn -1;
		float volt,curr;
		float backup_calibV = 1;
		float backup_calibC = 1;
//		float backup_calibE = 1;
		if(_calib_V || _calib_C){
			backup_calibV = PMT_GET_CALIB_V(chn_idx);
			backup_calibC = PMT_GET_CALIB_C(chn_idx);
//			backup_calibE = PMT_GET_CALIB_E(chn_idx);

			if(_calib_V) PMT_SET_CALIB_V(chn_idx,1); //set default
			if(_calib_C) PMT_SET_CALIB_C(chn_idx,1); //set default
			PMT_SET_CALIB_E(chn_idx,1); //set default
			stpm_read_rms_voltage_and_current(PMT_STRUCT[chn_idx],1,&volt,&curr);

//			PMT_STRUCT[chn_idx]->flag_calib = stpm_read_active_energy(PMT_STRUCT[chn_idx],1);//kWh //1;//0.01*_calip_E;

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
			//calculate calibE
//			delay_ms(100); ///delay to stpm refesh new data
			PMT_STRUCT[chn_idx]->flag_calib = 1000.0*stpm_read_active_power(PMT_STRUCT[chn_idx],1);
//			stpm_reset_energies(PMT_STRUCT[chn_idx]);
			blt_soft_timer_restart(_auto_calibE_collectData,1*1000*1000); //1s
		}
	}
}


void pmt_info(void *_arg, u8 _size){
	double* data = (double*)_arg;
	if(_size > 0 && data[0] > 0){
		P_INFO("Start interval read sensors:%d ms\r\n",(u32)data[0]);
		blt_soft_timer_restart(_interval_read_sensor,data[0]*1000);
		return;
	}
	else{
		blt_soft_timer_delete(_interval_read_sensor);
	}
	//cancel if automatical calibE running
	if(PMT_STRUCT[0]->flag_calib || PMT_STRUCT[1]->flag_calib || PMT_STRUCT[2]->flag_calib){
		ERR(PERI,"Automatic CalibE running.....\r\n");
		return;
	}

	PMT_LATCH_ALL();//important
	float period[3], volt[3], curr[3];
	float power[3], energy[3];
	float P_factor[3];
	P_INFO("*********************************************************\r\n");
	for (u8 chn = 0; chn < POWERMETER_CHANNEL; ++chn) {
		period[chn] = stpm_read_period(PMT_STRUCT[chn],1);
		stpm_read_rms_voltage_and_current(PMT_STRUCT[chn],1,&volt[chn],&curr[chn]);
		power[chn] =  1000.0*stpm_read_active_power(PMT_STRUCT[chn],1);
		energy[chn] = stpm_read_active_energy(PMT_STRUCT[chn],1);
		P_factor[chn] = stpm_read_power_factor(PMT_STRUCT[chn],1);
		P_INFO("[%d]F:%.1f,Vrms:%.3f,Irms:%.3f,Prms:%.6f,Pavg:%.3f,E:%.6f,PFrms:%.3f,PFavg:%d\r\n",
				chn,period[chn],volt[chn],curr[chn],power[chn],PMT_CTX[chn].p_avg,energy[chn],P_factor[chn],PMT_PF_AVG(chn));
	}
	P_INFO("*********************************************************\r\n");
}
void pmt_reset_energy(void *_arg, u8 _size) {
    u8 *args = (u8 *)_arg;  //
    memset(args+_size,0,6);
	u8 numofchn = args[0] == 0 ? POWERMETER_CHANNEL : args[0];
	PMT_LATCH_ALL();
	for (u8 chn = (args[0] > 0 ? args[0] - 1 : 0); chn < numofchn; ++chn) {
		ERR(USER,"[%d]Reset Energy:%.3f\r\n",chn,PMT_STRUCT[chn]->ph1_energy.active);
		PMT_CTX[chn].e_last_stored=0;
		stpm_reset_energies(PMT_STRUCT[chn]);
	    stpm_set_current_gain(PMT_STRUCT[chn],1,STPM32_GAIN);
	}
}
void pmt_settings(void *_arg, u8 _size) {
	//u8 *args = (u8 *) _arg;  //
	P_INFO("*** Settings:%d,%d,%d\r\n",PMT_GET_THRESHOLD(0),PMT_GET_THRESHOLD(1),PMT_GET_THRESHOLD(2));
}
static void pmt_calib(void *_arg, u8 _size) {
    double *args = (double *)_arg;  //
    memset(args+_size,0,6);
    LOGA(USER,"Calib cmd(%d):%.3lf, %.3lf,%.3lf,%.3lf,%.3lf,%.3lf\r\n",_size,args[0],args[1],args[2],args[3],args[4],args[5]);

    if(!_size){
    	pmt_calib_stpm(0,0,0,0,0); //get
    }
    else {
		PMT_LATCH_ALL();//important
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
static uint8_t stpm32_spi_transfer(uint8_t data)
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

static void stpm32_spi_transfer_buffer(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len) {
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

static void pmt_spi_begin(void){}
static void pmt_spi_end(void){}
static void pmt_spi_begin_transaction(void){}
static void pmt_spi_end_transaction(void){}
static void pmt_pin_write(int pin, bool state){gpio_set_level(pin, state);}
static void pmt_delay_us(uint32_t us){delay_us(us);}
static void pmt_delay_ms(uint32_t ms){delay_ms(ms);}

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

        PMT_STRUCT[i]->context = &PMT_CTX[i];

        if (!stpm_init(PMT_STRUCT[i]))
        {
        	ERR(PERI,"STPM32-%d Init err\n",i+1);
        }
        LOGA(PERI,"STPM32-%d Init ok!\n",i+1);
    }


    //Settings chip
    PMT_LATCH_ALL();
    stpm_set_current_gain(PMT_STRUCT[0],1,STPM32_GAIN);
    stpm_set_current_gain(PMT_STRUCT[1],1,STPM32_GAIN);
    stpm_set_current_gain(PMT_STRUCT[2],1,STPM32_GAIN);
    //	Load calib
    _pmt_load_calib();
	P_INFO("[1]Calib:U:%f,I:%f,E:%f\r\n",PMT_GET_CALIB_V(0),PMT_GET_CALIB_C(0),PMT_GET_CALIB_E(0));
	P_INFO("[2]Calib:U:%f,I:%f,E:%f\r\n",PMT_GET_CALIB_V(1),PMT_GET_CALIB_C(1),PMT_GET_CALIB_E(1));
	P_INFO("[3]Calib:U:%f,I:%f,E:%f\r\n",PMT_GET_CALIB_V(2),PMT_GET_CALIB_C(2),PMT_GET_CALIB_E(2));
	//Load Context
	_pmt_load_context();
	//=>update to stpm
	for (u8 chn = 0; chn < POWERMETER_CHANNEL; ++chn) {
		PMT_STRUCT[chn]->flag_calib=0;
		PMT_CTX[chn].i_avg=0;
		PMT_CTX[chn].p_avg=0;
//		PMT_STRUCT[chn]->ph1_energy.active = PMT_CTX[chn].e_sum / PMT_GET_CALIB_E(chn);
//		PMT_STRUCT[chn]->ph1_energy_hp.old_energy[0] = PMT_STRUCT[chn]->ph1_energy.active;
		P_INFO("[%d]Ectx:%f,Working-time:%d\r\n",chn,PMT_CTX[chn].e_last_stored,PMT_CTX[chn].working_time_ms);
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

pmt_cmd_t PMT_CMD[] = {
					{"calib",pmt_calib},
					{"readinfo",pmt_info},
					{"readsettings",pmt_settings},
					{"resetenergy",pmt_reset_energy}};

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

void pmt_reset_workingtime(u8 _chn) {

	PMT_CHECK_CHN(_chn);

	u8 numofchn = _chn == 0 ? POWERMETER_CHANNEL : _chn;
	for (u8 chn = (_chn > 0 ? _chn - 1 : 0); chn < numofchn; ++chn) {
		//debug
		P_INFO("[%d]F:%d,Uavg:%.3f,Iavg:%.3f,Pavg:%.6f,E:%.6f,PFavg:%d\r\n",chn,
						G_POWER_METER.data.frequency,PMT_CTX[chn].u_avg,
						PMT_CTX[chn].i_avg,
						PMT_CTX[chn].p_avg,PMT_CTX[chn].e_sum,
						PMT_PF_AVG(chn) & 0x7F);
		//
		LOGA(APP,"[%d]Reset Working-time:%d ms\r\n",chn,PMT_CTX[chn].working_time_ms);
		PMT_CTX[chn].working_time_ms=0;
	}
}
/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
static uint32_t calc_I(float _curr)
{
    uint32_t result;
	if (_curr*1000 > 1023) {
	    _curr = roundf(_curr*100.0)/100.0;
		result = (uint32_t)(_curr*10);
		result |= 0x80000000;         //
	} else {
	    _curr = roundf(_curr*10000.0)/10000.0;
		result = (uint32_t)(_curr*1000);
		result &= 0x7FFFFFFF;         //
	}
//    P_INFO("I:%ld %s|"PRINTF_BINARY_PATTERN_INT32"\r\n",result&0x7FFFFFFF,((result & 0x80000000)>0)?"A":"mA",PRINTF_BYTE_TO_BINARY_INT32(result));
    return result;
}
static u8 calc_FP(u8 _chn) {
	if(_chn >=POWERMETER_CHANNEL){
		return 0;
	}
	float PF_rslt = 0;
	u32 volt_avg = 1;
	u32 cur_avg = 1;
	u32 p_avg = 1;
	volt_avg=(u32)(10*PMT_CTX[_chn].u_avg);
	if (PMT_CTX[_chn].i_avg * 1000 > 1023) {
		cur_avg = (u32)(10*roundf(PMT_CTX[_chn].i_avg * 100.0) / 100.0);   //
		p_avg=(u32)(PMT_CTX[_chn].p_avg*100);
	} else {
		cur_avg = (u32)(1000*roundf(PMT_CTX[_chn].i_avg * 10000.0) / 10000.0);       //
		p_avg=(u32)(PMT_CTX[_chn].p_avg*10000);
	}

	if(volt_avg && cur_avg && p_avg){
		PF_rslt = 100.0*p_avg/(volt_avg*cur_avg);
	}
//	 P_INFO("[%d]PF:%d/%d*%d=%f\r\n",_chn,p_avg,volt_avg,cur_avg,PF_rslt);
	return (u8)roundf((PF_rslt*10.0)/10.0);
}

void pmt_main(void){
	static u32 sample_timing = 0;
	static u32 save_context = 0;
	float curr=0;
	float volt=0;
	u32 i_cal =0;
	u8 current_unit = 0;
	u8 chn_update=0;
	float p_rms = 0;
	if (!PMT_CALIB_RUNNING()) {
		if (clock_time_exceed(sample_timing,PMT_SAMPLE_TIMING * 1000)) {
			PMT_DEBUG_STOP()
			//process payload for packet
			PMT_LATCH_ALL();
			G_POWER_METER.data.frequency = stpm_read_period(PMT_STRUCT[0],1);
			stpm_read_rms_voltage_and_current(PMT_STRUCT[0],1,&volt,&curr);
			//=========== Chn1
			chn_update = 0;
			PMT_CTX[chn_update].u_avg = (volt+PMT_CTX[chn_update].u_avg)/2;
			G_POWER_METER.data.voltage = (u16)PMT_CTX[chn_update].u_avg;
			PMT_CTX[chn_update].i_avg = (PMT_CTX[chn_update].i_avg + curr) / 2;
			i_cal = calc_I(PMT_CTX[chn_update].i_avg);
			G_POWER_METER.data.current1 = (u16) (i_cal & 0x3FF);
			current_unit = ((i_cal >> 31) & 0x1) << 7;
			p_rms = 1000.0 * stpm_read_active_power(PMT_STRUCT[chn_update],1);
			PMT_CTX[chn_update].p_avg = (PMT_CTX[chn_update].p_avg + p_rms) / 2;
			G_POWER_METER.data.fac_power1 = calc_FP(chn_update) | current_unit;
			//(((u8) (100.0 * stpm_read_power_factor(PMT_STRUCT[chn_update],1)) + (G_POWER_METER.data.fac_power1 & 0x7F)) / 2) | current_unit;
			PMT_CTX[chn_update].e_sum = PMT_CTX[chn_update].e_last_stored + stpm_read_active_energy(PMT_STRUCT[chn_update],1);
			G_POWER_METER.data.energy1 = (u32) (PMT_CTX[chn_update].e_sum * 10);

			if ((u16) p_rms > PMT_GET_THRESHOLD(chn_update)) {
				PMT_CTX[chn_update].working_time_ms +=
						(clock_time() - sample_timing > 0) ? (clock_time() - sample_timing) / SYSTEM_TIMER_TICK_1MS : PMT_SAMPLE_TIMING;
			}
			G_POWER_METER.data.time1 = (u8) ceilf((float)PMT_CTX[chn_update].working_time_ms *1.0/ 1000.0);
			//============ Chn 2
			chn_update = 1;
			stpm_read_rms_voltage_and_current(PMT_STRUCT[chn_update],1,&volt,&curr);
			PMT_CTX[chn_update].u_avg = (volt+PMT_CTX[chn_update].u_avg)/2;
			PMT_CTX[chn_update].i_avg = (PMT_CTX[chn_update].i_avg + curr) / 2;
			i_cal = calc_I(PMT_CTX[chn_update].i_avg);
			G_POWER_METER.data.current2 = (u16) (i_cal & 0x3FF) ;
			current_unit = ((i_cal >> 31) & 0x1) << 7;
			p_rms = 1000.0 * stpm_read_active_power(PMT_STRUCT[chn_update],1);
			PMT_CTX[chn_update].p_avg = (PMT_CTX[chn_update].p_avg + p_rms) / 2;
			G_POWER_METER.data.fac_power2 = calc_FP(chn_update) | current_unit;
			//(((u8) (100.0 * stpm_read_power_factor(PMT_STRUCT[chn_update],1)) + (G_POWER_METER.data.fac_power2 & 0x7F)) / 2) | current_unit;

			PMT_CTX[chn_update].e_sum = PMT_CTX[chn_update].e_last_stored + stpm_read_active_energy(PMT_STRUCT[chn_update],1);
			G_POWER_METER.data.energy2 = (u32) (PMT_CTX[chn_update].e_sum * 10);
			if ((u16) p_rms > PMT_GET_THRESHOLD(chn_update)) {
				PMT_CTX[chn_update].working_time_ms +=
						(clock_time() - sample_timing > 0) ? (clock_time() - sample_timing) / SYSTEM_TIMER_TICK_1MS : PMT_SAMPLE_TIMING;
			}
			G_POWER_METER.data.time2 = (u8) ceilf((float)PMT_CTX[chn_update].working_time_ms *1.0/ 1000.0);
			//============ Chn 3
			chn_update = 2;
			stpm_read_rms_voltage_and_current(PMT_STRUCT[chn_update],1,&volt,&curr);
			PMT_CTX[chn_update].u_avg = (volt+PMT_CTX[chn_update].u_avg)/2;
			PMT_CTX[chn_update].i_avg = (PMT_CTX[chn_update].i_avg + curr) / 2;
			i_cal = calc_I(PMT_CTX[chn_update].i_avg);
			G_POWER_METER.data.current3 = (u16) (i_cal & 0x3FF);
			current_unit = ((i_cal >> 31) & 0x1) << 7;
			p_rms = 1000.0 * stpm_read_active_power(PMT_STRUCT[chn_update],1);
			PMT_CTX[chn_update].p_avg = (PMT_CTX[chn_update].p_avg + p_rms) / 2;
			G_POWER_METER.data.fac_power3 = calc_FP(chn_update) | current_unit;
			//(((u8) (100.0 * stpm_read_power_factor(PMT_STRUCT[chn_update],1)) + (G_POWER_METER.data.fac_power3 & 0x7F)) / 2) | current_unit;
			PMT_CTX[chn_update].e_sum = PMT_CTX[chn_update].e_last_stored + stpm_read_active_energy(PMT_STRUCT[chn_update],1);
			G_POWER_METER.data.energy3 = (u32) (PMT_CTX[chn_update].e_sum * 10);
			if ((u16) p_rms > PMT_GET_THRESHOLD(chn_update)) {
				PMT_CTX[chn_update].working_time_ms +=
						(clock_time() - sample_timing > 0) ? (clock_time() - sample_timing) / SYSTEM_TIMER_TICK_1MS : PMT_SAMPLE_TIMING;
			}
			G_POWER_METER.data.time3 = (u8) ceilf((float)PMT_CTX[chn_update].working_time_ms *1.0/ 1000.0);

			//update timing
			sample_timing = clock_time();
			//restart if turn on debug
			PMT_DEBUG()

		}
		//storage context
		if (clock_time_exceed(save_context,5 * 999 * 1001)) {
			_pmt_save_context();
			save_context = clock_time();
		}
	}
}

#endif //POWER_METER_DEVICE

/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_nwk_protocol.c
 *Created on		: Jul 23, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_nwk_protocol.h"
#include "fl_nwk_database.h"
#include "fl_nwk_handler.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

#ifdef MASTER_CORE
#define CMDLINE_MAXLEN	20
typedef struct {
	u8 c_cmd[CMDLINE_MAXLEN];
	u8 len;
	void (*ExcuteFunc)(u8*);
}__attribute__((packed)) fl_cmdlines_t;

fl_slave_getinfo_t G_SLA_INFO; //for cmdlines

extern fl_slaves_list_t G_NODE_LIST;
//For getting automatic information
#define GETINFO_1_TIMES_MAX			20
#define GETINFO_FREQUENCY			20 //ms
#define GETINFO_FIRST_DUTY			25*1000//s
typedef struct {
	fl_nodeinnetwork_t* id[GETINFO_1_TIMES_MAX];
	u8 num_retrieved; // number of slaves retrieved out
	u8 total_slaves; //
	u8 num_1_times;
	u8 num_retry;
	u32 timeout_rsp_start;
	u32 time_start;
	struct {
			u8 time_interval; //second
			u8 total_slaves;
			u8 num_1_times; //
			u8 num_retry;
			u16 timeout_rsp; //ms
	}settings;
	struct {
		u8 num_onl;
		u8 num_off;
		fl_nodeinnetwork_t* offline[MAX_NODES];
		u32 total_time;
	} rslt;
}__attribute__((packed)) fl_master_getinfo_pointer_t;

fl_master_getinfo_pointer_t G_SLA_INFO_RSP = { .num_1_times = 0, .num_retrieved = 0xFF, .time_start = 0, .total_slaves = 0 };

#define CLEAR_INFO_RSP() 	do { \
								G_SLA_INFO_RSP.num_1_times = 0; \
								G_SLA_INFO_RSP.time_start = 0; \
								G_SLA_INFO_RSP.total_slaves = 0; \
								G_SLA_INFO_RSP.num_retrieved = 0xFF; \
								G_SLA_INFO_RSP.num_retry = 0; \
								G_SLA_INFO_RSP.timeout_rsp_start=0;\
								G_SLA_INFO_RSP.settings.time_interval = 0; \
								G_SLA_INFO_RSP.settings.timeout_rsp = 0; \
								G_SLA_INFO_RSP.settings.num_1_times = 0; \
								G_SLA_INFO_RSP.settings.num_retry = 0; \
								G_SLA_INFO_RSP.settings.total_slaves = 0; \
								G_SLA_INFO_RSP.rslt.num_onl = 0;\
								G_SLA_INFO_RSP.rslt.num_off = 0;\
								G_SLA_INFO_RSP.rslt.total_time = 0;\
							} while(0)

/********************* Functions SET CMD declare ********************/
void CMD_SETUTC(u8* _data);
void CMD_INSTALLMODE(u8* _data);
void CMD_DEBUG(u8* _data);
void CMD_HEARTBEAT(u8* _data);
void CMD_REPEAT(u8* _data);
void CMD_ADVINTERVAL(u8* _data);
void CMD_ADVSCAN(u8* _data);
void CMD_CLEARDB(u8* _data);
void CMD_CHANNELCONFIG(u8* _data);
/********************* Functions GET CMD declare ********************/
void CMD_GETSLALIST(u8* _data);
void CMD_GETINFOSLAVE(u8* _data);
void CMD_GETADVSETTING(u8* _data);

fl_cmdlines_t G_CMDSET[] = { { { 'u', 't', 'c' }, 3, CMD_SETUTC }, 			// p set utc yymmddhhmmss
		{ { 'i', 'n', 's', 't', 'a', 'l', 'l' }, 7, CMD_INSTALLMODE },		// p install on/off
		{ { 'd', 'e', 'b', 'u', 'g' }, 5, CMD_DEBUG },						// p set debug on/off : use to set for the slaves
		{ { 'h', 'b' }, 2, CMD_HEARTBEAT },									// p set hb <time> : use to set period time for heartbeat packet
		{ { 'r', 'e', 'p', 'e', 'a', 't' }, 5, CMD_REPEAT },				// p set repeat <ttl count> : use to set repeat-mode for slaves (0 :off)
		{ { 'a', 'd', 'v' }, 3, CMD_ADVINTERVAL },							// p set adv <interval_min> <interval_max) : use to set adv settings
		{ { 's', 'c', 'a', 'n' }, 4, CMD_ADVSCAN },							// p set scan <window> <interval> : use to set scanner adv settings
		{ { 'c', 'l', 'e', 'a', 'r' }, 5, CMD_CLEARDB },					// p set clear <nodelist>
		{ { 'c', 'h', 'n' }, 3, CMD_CHANNELCONFIG },						// p set chn <chn1> <chn2> <chn3>
		};

fl_cmdlines_t G_CMDGET[] = { { { 's', 'l', 'a', 'l', 'i', 's', 't' }, 7, CMD_GETSLALIST },		// p get list
		{ { 'i', 'n', 'f', 'o' }, 4, CMD_GETINFOSLAVE },					// p get <id8> <id8> .... : max = 16 id
		{ { 'a', 'd', 'v', 'c', 'o', 'n', 'f', 'i', 'g' }, 9, CMD_GETADVSETTING },					// p get scan : use to get scanner adv settings
		// p get info <mac_u32>
		};


static u8 FIRST_PROTOCOL_START = 0;
#endif

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE

/***************************************************
 * @brief 		:automatically get all information of slaves
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int _getInfo_autorun(void) {
	//debug
	datetime_t cur_dt;
	u32 cur_timetamp = fl_rtc_get();
	fl_rtc_timestamp_to_datetime(cur_timetamp,&cur_dt);
	//1. get real total slaves in the container
	u8 slave_arr[GETINFO_1_TIMES_MAX];
	u8 var = 0;
	//check rsp of the slaves
	u8 idx_get = 0;
	u8 get_num_reported = 0;
	static u8 slot_offline = 0;
	if (G_SLA_INFO_RSP.num_retrieved != 0xFF) {
		for (idx_get = 0; idx_get < G_SLA_INFO_RSP.num_1_times; ++idx_get) {
			if (G_SLA_INFO_RSP.id[idx_get]->active == true) {
				get_num_reported++;
				if (get_num_reported == G_SLA_INFO_RSP.num_1_times) {
					goto NEXT_STEP;
				}
			}
		}
		//Check timeout expired
		if (clock_time_exceed(G_SLA_INFO_RSP.timeout_rsp_start,(G_SLA_INFO_RSP.settings.num_1_times)* G_SLA_INFO_RSP.settings.timeout_rsp * 1000)) { //convert to ms
			//store offline
			for (idx_get = 0; idx_get < G_SLA_INFO_RSP.num_1_times; ++idx_get) {
				if (G_SLA_INFO_RSP.id[idx_get]->active == false) {
					G_SLA_INFO_RSP.rslt.offline[slot_offline++] = G_SLA_INFO_RSP.id[idx_get];
				}
			}
			//Check complete
			if (G_SLA_INFO_RSP.num_retrieved >= G_SLA_INFO_RSP.total_slaves) {
				G_SLA_INFO_RSP.rslt.num_onl += get_num_reported;
				G_SLA_INFO_RSP.timeout_rsp_start = clock_time();
				goto OUTPUT_RESULT;
			}
			//continue to get
			goto NEXT_STEP;
		}else return GETINFO_FREQUENCY*1000;
	} else
	{
		G_SLA_INFO_RSP.num_retrieved = 0;
		if(G_SLA_INFO_RSP.time_start ==0){
			P_INFO("[%02d/%02d/%02d-%02d:%02d:%02d]Start GetInfo!!\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
			G_SLA_INFO_RSP.time_start = clock_time();
		}
	}
//======== NEXT
	NEXT_STEP:
	//check rsp and full data
	//LOGA(INF,"get_num_reported:%d\r\n",get_num_reported);
	G_SLA_INFO_RSP.rslt.num_onl += get_num_reported;
	//LOGA(INF,"get_num_reported:%d-%d/%d\r\n",get_num_reported,G_SLA_INFO_RSP.rslt.num_onl,G_SLA_INFO_RSP.total_slaves);
	G_SLA_INFO_RSP.timeout_rsp_start = clock_time();
	if (G_SLA_INFO_RSP.rslt.num_onl == G_SLA_INFO_RSP.total_slaves)
		goto OUTPUT_RESULT;
	//next slaves
	for (var = 0; var < G_SLA_INFO_RSP.num_1_times && var + G_SLA_INFO_RSP.num_retrieved < G_SLA_INFO_RSP.total_slaves; ++var) {
		if(G_SLA_INFO_RSP.num_retry == 0){
			G_SLA_INFO_RSP.id[var] = &G_NODE_LIST.sla_info[(var + G_SLA_INFO_RSP.num_retrieved)%G_NODE_LIST.slot_inused];
		}
		else{
			G_SLA_INFO_RSP.id[var] = G_SLA_INFO_RSP.rslt.offline[(var + G_SLA_INFO_RSP.num_retrieved)%G_SLA_INFO_RSP.rslt.num_off];
		}
		slave_arr[var] = G_SLA_INFO_RSP.id[var]->slaveID.id_u8;
		//clear status of slave
		G_SLA_INFO_RSP.id[var]->active = false;
		G_SLA_INFO_RSP.id[var]->timelife = clock_time();
	}

	if(var == 0) goto OUTPUT_RESULT; //done

	LOGA(DRV,"GetInfo: %d->%d/%d\r\n",G_SLA_INFO_RSP.num_retrieved,var + G_SLA_INFO_RSP.num_retrieved - 1,G_SLA_INFO_RSP.total_slaves);
	P_INFO("Loading...(%d->%d/%d)\r\n",G_SLA_INFO_RSP.num_retrieved,var + G_SLA_INFO_RSP.num_retrieved - 1,G_SLA_INFO_RSP.total_slaves);
	fl_pack_t info_pack = fl_master_packet_GetInfo_build(slave_arr,var);
	P_PRINTFHEX_A(DRV,info_pack.data_arr,info_pack.length,"%s(%d):","Info Pack",info_pack.length);
	fl_adv_sendFIFO_add(info_pack);
	//update retrieved slaves
	G_SLA_INFO_RSP.num_retrieved = var + G_SLA_INFO_RSP.num_retrieved;
	if (G_SLA_INFO_RSP.num_retrieved > G_SLA_INFO_RSP.total_slaves)
		G_SLA_INFO_RSP.num_retrieved = G_SLA_INFO_RSP.total_slaves;
	if (var < G_SLA_INFO_RSP.num_1_times)
		G_SLA_INFO_RSP.num_1_times = var;

	return GETINFO_FREQUENCY*1000;
//======== OUTPUT
	OUTPUT_RESULT:
	P_INFO("**Online :%d/%d\r\n",G_SLA_INFO_RSP.rslt.num_onl,G_SLA_INFO_RSP.total_slaves);
	if (G_SLA_INFO_RSP.rslt.num_onl < G_SLA_INFO_RSP.total_slaves) {
		G_SLA_INFO_RSP.rslt.num_off = G_SLA_INFO_RSP.total_slaves - G_SLA_INFO_RSP.rslt.num_onl;
		P_INFO("**Offline:%d/%d\r\n",G_SLA_INFO_RSP.rslt.num_off,G_SLA_INFO_RSP.total_slaves);
		for (idx_get = 0; idx_get < G_SLA_INFO_RSP.rslt.num_off; ++idx_get) {
			P_INFO("[%d]Mac:0x%02X%02X%02X%02X%02X%02X\r\n",G_SLA_INFO_RSP.rslt.offline[idx_get]->slaveID.id_u8,
					G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[0],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[1],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[2],
					G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[3],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[4],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[5]);
		}
// ======= RETRY
		if(G_SLA_INFO_RSP.num_retry < G_SLA_INFO_RSP.settings.num_retry){
			//for debuging
			//PLOG_Start(INF);
			G_SLA_INFO_RSP.num_retry++;
			G_SLA_INFO_RSP.num_retrieved = 0xFF;
			G_SLA_INFO_RSP.total_slaves = G_SLA_INFO_RSP.rslt.num_off;
			G_SLA_INFO_RSP.num_1_times = (G_SLA_INFO_RSP.settings.num_1_times<G_SLA_INFO_RSP.total_slaves)?G_SLA_INFO_RSP.settings.num_1_times:G_SLA_INFO_RSP.rslt.num_off;
			G_SLA_INFO_RSP.rslt.num_onl = 0;
			G_SLA_INFO_RSP.rslt.num_off = 0;
			slot_offline = 0;
			P_INFO("#Retry offline list:%d/%d\r\n",G_SLA_INFO_RSP.num_retry,G_SLA_INFO_RSP.settings.num_retry);
			return GETINFO_FREQUENCY*1000;
		}
		ERR(DRV,"**Offline:%d/%d\r\n",G_SLA_INFO_RSP.rslt.num_off,G_SLA_INFO_RSP.total_slaves);
		for (idx_get = 0; idx_get < G_SLA_INFO_RSP.rslt.num_off; ++idx_get) {
			ERR(DRV,"[%d]Mac:0x%02X%02X%02X%02X%02X%02X\r\n",G_SLA_INFO_RSP.rslt.offline[idx_get]->slaveID.id_u8,
					G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[0],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[1],
					G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[2],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[3],
					G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[4],G_SLA_INFO_RSP.rslt.offline[idx_get]->mac[5]);
		}
	}
	//PLOG_Stop(INF);
	P_INFO("**RTT    :%d ms\r\n",(clock_time() - G_SLA_INFO_RSP.time_start)/SYSTEM_TIMER_TICK_1MS);
	P_INFO("[%02d/%02d/%02d-%02d:%02d:%02d]End GetInfo!!\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
	P_INFO("=========================================\r\n");
	//Clear and Restart get all
	G_SLA_INFO_RSP.num_retrieved = 0xFF;
	G_SLA_INFO_RSP.rslt.num_onl = 0;
	G_SLA_INFO_RSP.rslt.num_off = 0;
	G_SLA_INFO_RSP.time_start = 0;
	G_SLA_INFO_RSP.num_1_times = G_SLA_INFO_RSP.settings.num_1_times;
	G_SLA_INFO_RSP.total_slaves = G_SLA_INFO_RSP.settings.total_slaves;
	G_SLA_INFO_RSP.num_retry = 0;
	slot_offline = 0;
	return (G_SLA_INFO_RSP.settings.time_interval > 1 ? G_SLA_INFO_RSP.settings.time_interval*1000*1000 : -1);
}

/***************************************************
 * @brief 		:timer callback checking rsp for cmdline getting info
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
int _RSP_CMD_GETINFOSLAVE(void) {
	s16 slave_idx = -1;		//
	u8 num_rsp = 0;
	u32 max_time = 0;
	u32 total_time = 0;

	for (int var = 0; var < G_SLA_INFO.num_sla; ++var) {
		slave_idx = fl_master_SlaveID_find(G_SLA_INFO.id[var]);
		if (slave_idx != -1) {
			num_rsp = (G_NODE_LIST.sla_info[slave_idx].active) ? num_rsp + 1 : num_rsp;
			if (max_time < G_NODE_LIST.sla_info[slave_idx].timelife) {
				max_time = G_NODE_LIST.sla_info[slave_idx].timelife;
			}
			//total_time=total_time + G_NODE_LIST.sla_info[slave_idx].timelife;
		}
	}
	if (num_rsp == G_SLA_INFO.num_sla) {
		P_INFO("Online :%d/%d\r\n",num_rsp,G_SLA_INFO.num_sla);
		//P_INFO("LongestTime:%d ms\r\n",max_time/SYSTEM_TIMER_TICK_1MS);
		total_time = (clock_time() - G_SLA_INFO.timetamp) / SYSTEM_TIMER_TICK_1MS;
		P_INFO("RspTime:%d ms\r\n",total_time);
		return -1;
	}
	return 0;
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/
int REBOOT_DEV(void) {
	LOGA(APP,"RTC saved!\r\n");
	fl_db_all_save();
	//
	sys_reboot();
	return -1;
}
#ifdef MASTER_CORE
s8 CMD_EXCUTE_FUNC(fl_cmdlines_t *_pGcontainer, u8 _Gcontainer_size, u8* _data) {
	s8 index_cmd = -1;
	for (u8 idx = 0; idx < _Gcontainer_size; ++idx) {
		index_cmd = plog_IndexOf(_data,_pGcontainer[idx].c_cmd,_pGcontainer[idx].len,CMDLINE_MAXLEN);
		if (index_cmd != -1) {
			_pGcontainer[idx].ExcuteFunc(_data + index_cmd);
			return idx;
		}
	}
	return -1;
}
/********************* Functions SET CMD ********************/
void CMD_SETUTC(u8* _data) {
	u32 timetamp_set = 0;
	datetime_t cur_dt;
	cur_dt = fl_parse_datetime(_data);
	LOGA(DRV,"TIME SET:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
	timetamp_set = fl_rtc_datetime_to_timestamp(&cur_dt);
	fl_rtc_set(timetamp_set);
}
void CMD_INSTALLMODE(u8* _data) {
	extern volatile u8 MASTER_INSTALL_STATE;
	u8 ON[2] = { 'o', 'n' };
	u8 on_bool = 0;
	if (plog_IndexOf(_data,ON,2,CMDLINE_MAXLEN) != -1) {
		on_bool = 1;
	}
	MASTER_INSTALL_STATE = on_bool;
	LOGA(DRV,"Master installMode: %s\r\n",(on_bool) ? "ON" : "OFF");
}
void CMD_DEBUG(u8* _data) {
	extern volatile u8 NWK_DEBUG_STT;
	u8 ON[2] = { 'o', 'n' };
	u8 on_bool = 0;
	if (plog_IndexOf(_data,ON,2,CMDLINE_MAXLEN) != -1) {
		on_bool = 1;
	}
	NWK_DEBUG_STT = on_bool;
	LOGA(DRV,"Master Netrwork Debug: %s\r\n",(on_bool) ? "ON" : "OFF");
}

void CMD_HEARTBEAT(u8* _data) {
	extern u16 PERIOD_HEARTBEAT;
	u16 period_hb = 0;
	//p set hb 5000
	int rslt = sscanf((char*) _data,"hb %hd",&period_hb);
	if (rslt == 1) {
		if (period_hb > 1000) {
			LOGA(DRV,"HeartBeat Period:%d ms\r\n",period_hb);
			PERIOD_HEARTBEAT = period_hb;
			return;
		} else if (period_hb == 0) {
			LOG_P(DRV,"HeartBeat OFF\r\n");
			PERIOD_HEARTBEAT = period_hb;
			return;
		}
	}
	ERR(DRV,"ERR HeartBeat Period (%d):%d\r\n",rslt,period_hb);

}
void CMD_REPEAT(u8* _data) {
//	extern volatile u8 NWK_REPEAT_MODE;
//	u8 ON[2] = { 'o', 'n' };
//	u8 on_bool = 0;
//	if (plog_IndexOf(_data,ON,2,CMDLINE_MAXLEN) != -1) {
//		on_bool = 1;
//	}
//	NWK_REPEAT_MODE = on_bool;
	extern volatile u8  REPEAT_LEVEL;
	u16 repeat_cnt = 0;
	//p set repeat 2
	int rslt = sscanf((char*) _data,"repeat %hd",&repeat_cnt);
	if(rslt == 1){
		REPEAT_LEVEL = repeat_cnt;
		LOGA(DRV,"Repeat Mode: %d\r\n",REPEAT_LEVEL);
	}
}
void CMD_ADVINTERVAL(u8* _data) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	u16 interval_min = 0;
	u16 interval_max = 0;
	//p set adv 20 25
	int rslt = sscanf((char*) _data,"adv %hd %hd",&interval_min,&interval_max);
	if (rslt == 2) {
		if (interval_max >= interval_min && interval_min >= 20) {
			LOGA(DRV,"ADV setting => min:%d |max :%d\r\n",interval_min,interval_max);
			G_ADV_SETTINGS.adv_interval_min = interval_min / 0.625;
			G_ADV_SETTINGS.adv_interval_max = interval_max / 0.625;
			return;
		}
	}
	ERR(DRV,"ERR ADV settings (%d)\r\n",rslt);
}
void CMD_ADVSCAN(u8* _data) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	u16 scan_interval = 0;
	u16 scan_window = 0;
	//p set scan 50 50
	int rslt = sscanf((char*) _data,"scan %hd %hd",&scan_window,&scan_interval);
	if (rslt == 2) {
		if (scan_interval >= scan_window && scan_window >= 50) {
			LOGA(DRV,"Scanner setting => window:%d |Interval :%d\r\n",scan_window,scan_interval);
			G_ADV_SETTINGS.scan_window = scan_window / 0.625;
			G_ADV_SETTINGS.scan_interval = scan_interval / 0.625;
			fl_adv_setting_update();
			return;
		}
	}
	ERR(DRV,"ERR Scanner settings (%d)\r\n",rslt);
}
void CMD_CLEARDB(u8* _data) {
	//p get clear slalist
	u8 nodelist_c[8] = { 's', 'l', 'a', 'l', 'i', 's', 't' };
	int rslt = plog_IndexOf(_data,nodelist_c,sizeof(nodelist_c),CMDLINE_MAXLEN);
	if (rslt != -1) {
		LOG_P(DRV,"Clear NODELIST DB\r\n");
		fl_db_nodelist_clearAll();
		REBOOT_DEV();
	}
}
void CMD_CHANNELCONFIG(u8* _data) {
	extern fl_master_config_t G_MASTER_INFO;
	u16 channels[3] = {0,0,0};
	//p set chn 10 11 12
	int rslt = sscanf((char*) _data,"chn %hd %hd %hd",&channels[0],&channels[1],&channels[2]);
	if (rslt == 3) {
		G_MASTER_INFO.nwk.chn[0] = channels[0];
		G_MASTER_INFO.nwk.chn[1] = channels[1];
		G_MASTER_INFO.nwk.chn[2] = channels[2];
		LOGA(DRV,"Channels setting:%d |%d |%d\r\n",G_MASTER_INFO.nwk.chn[0],G_MASTER_INFO.nwk.chn[1],G_MASTER_INFO.nwk.chn[2]);
		return;
	}
	ERR(DRV,"ERR Channels setting (%d)\r\n",rslt);
}
/********************* Functions GET CMD ********************/
void CMD_GETSLALIST(u8* _data) {
	fl_nodeinnetwork_t _node;
	MAC_ZERO_CLEAR(_node.mac,0);
	fl_master_nodelist_AddRefesh(_node);
	extern fl_slaves_list_t G_NODE_LIST;
	void _master_nodelist_printf(fl_slaves_list_t *_node, u8 _size);
	_master_nodelist_printf(&G_NODE_LIST,G_NODE_LIST.slot_inused);
}
void CMD_GETADVSETTING(u8* _data) {
	extern fl_adv_settings_t G_ADV_SETTINGS;
	LOG_P(DRV,"***** ADV Settings *****\r\n");
	LOGA(DRV,"ADV interval:%d-%d|%d\r\n",(u8 )(G_ADV_SETTINGS.adv_interval_min * 0.625),(u8 )(G_ADV_SETTINGS.adv_interval_max * 0.625),
			G_ADV_SETTINGS.adv_duration);
	LOGA(DRV,"ADV scanner :%d-%d\r\n",(u8 )(G_ADV_SETTINGS.scan_window * 0.625),(u8 )(G_ADV_SETTINGS.scan_window * 0.625));
	LOGA(DRV,"Channels: %d |%d |%d \r\n",*G_ADV_SETTINGS.nwk_chn.chn1,*G_ADV_SETTINGS.nwk_chn.chn2,*G_ADV_SETTINGS.nwk_chn.chn3);
	LOG_P(DRV,"************************\r\n");
}
void CMD_GETINFOSLAVE(u8* _data) {
	extern fl_adv_settings_t G_ADV_SETTINGS ;
	u8 slaveID[GETINFO_1_TIMES_MAX]; //Max 20 slaves
	int slave_num = sscanf((char*) _data,"info %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd %hd",&slaveID[0],
			&slaveID[1],&slaveID[2],&slaveID[3],&slaveID[4],&slaveID[5],&slaveID[6],&slaveID[7],&slaveID[8],&slaveID[9],&slaveID[10],&slaveID[11],
			&slaveID[12],&slaveID[13],&slaveID[14],&slaveID[15],&slaveID[16],&slaveID[17],&slaveID[18],&slaveID[19]);
	//p get info 255 <Period get again> <num slave for each> <num virtual slave> <timeout rsp>
	if(G_NODE_LIST.slot_inused == 0xFF) return;
	if (slave_num == 6 && slaveID[0] == 0xFF) {
		CLEAR_INFO_RSP();
		G_SLA_INFO_RSP.settings.time_interval = slaveID[1];

		G_SLA_INFO_RSP.settings.num_1_times = slaveID[2];
		G_SLA_INFO_RSP.num_1_times = G_SLA_INFO_RSP.settings.num_1_times;

		G_SLA_INFO_RSP.settings.total_slaves = slaveID[3] > MAX_NODES ? MAX_NODES : slaveID[3];
		G_SLA_INFO_RSP.total_slaves = G_SLA_INFO_RSP.settings.total_slaves;

		G_SLA_INFO_RSP.settings.timeout_rsp = slaveID[4]+100;
		G_SLA_INFO_RSP.settings.num_retry = slaveID[5];

		//SETTING global
		G_ADV_SETTINGS.time_wait_rsp = G_SLA_INFO_RSP.settings.timeout_rsp;
		G_ADV_SETTINGS.retry_times = G_SLA_INFO_RSP.settings.num_retry;

		LOGA(DRV,"GET ALL INFO AUTORUN (interval:%d s|NumOfTimes:%d |Total:%d |Timeout:%d ms|Retry:%d)!!\r\n",G_SLA_INFO_RSP.settings.time_interval,
				G_SLA_INFO_RSP.settings.num_1_times,
				G_SLA_INFO_RSP.settings.total_slaves,
				G_ADV_SETTINGS.time_wait_rsp,
				G_ADV_SETTINGS.retry_times);
		//create timer checking to manage response
		//Clear and re-start
		blt_soft_timer_delete(&_getInfo_autorun);
		blt_soft_timer_add(&_getInfo_autorun,(FIRST_PROTOCOL_START == 0 ? 1000 : GETINFO_FIRST_DUTY) * 1000); //ms
	} else if (slave_num >= 1) {
		//Clear and re-start
		blt_soft_timer_delete(&_getInfo_autorun);
		CLEAR_INFO_RSP();
		P_PRINTFHEX_A(DRV,slaveID,slave_num,"num(%d):",slave_num);
		fl_pack_t info_pack = fl_master_packet_GetInfo_build(slaveID,slave_num);
		P_PRINTFHEX_A(DRV,info_pack.data_arr,info_pack.length,"%s(%d):","Info Pack",info_pack.length);
		fl_adv_sendFIFO_add(info_pack);
		G_SLA_INFO.num_sla = slave_num;
		G_SLA_INFO.timetamp = clock_time();
		memcpy(G_SLA_INFO.id,slaveID,slave_num);
		//create timer checking to manage response
		if (blt_soft_timer_find(&_RSP_CMD_GETINFOSLAVE) == -1) {
			blt_soft_timer_add(&_RSP_CMD_GETINFOSLAVE,GETINFO_FREQUENCY * 1000); //ms
		}
	}
}
#endif
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/
#ifdef MASTER_CORE
void _Passing_CmdLine(type_debug_t _type, u8 *_data) {
	LOGA(DRV,"CMD(%d):%s\r\n",_type,_data);
	if (_type == SETCMD) {
		CMD_EXCUTE_FUNC(G_CMDSET,sizeof(G_CMDSET) / sizeof(fl_cmdlines_t),_data);
	} else if (_type == GETCMD) {
		CMD_EXCUTE_FUNC(G_CMDGET,sizeof(G_CMDGET) / sizeof(fl_cmdlines_t),_data);
	} else if (_type == RSTCMD) {
		ERR(APP,"Device will reset after 3s !!!!\r\n");
		blt_soft_timer_add(&REBOOT_DEV,3 * 1000 * 1000);
	}else if (_type == FACTORYCMD) {
		ERR(APP,"Clear and reset factory.....\r\n");
		fl_db_clearAll();
		delay_ms(1000);
		sys_reboot();
	}//
}

void fl_nwk_protcol_ExtCall(type_debug_t _type, u8 *_data){
	_Passing_CmdLine(_type,_data);
}

void fl_nwk_protocol_InitnRun(void){
	extern fl_adv_settings_t G_ADV_SETTINGS ;
	char cmd_fmt[50];
	memset((u8*)cmd_fmt,0,SIZEU8(cmd_fmt));
	sprintf(cmd_fmt,"p get info %d %d %d %d %d %d",255,0,8,G_NODE_LIST.slot_inused,G_ADV_SETTINGS.time_wait_rsp,G_ADV_SETTINGS.retry_times);
	_Passing_CmdLine(GETCMD,(u8*)cmd_fmt);
	FIRST_PROTOCOL_START =1; //don't change
}

#endif

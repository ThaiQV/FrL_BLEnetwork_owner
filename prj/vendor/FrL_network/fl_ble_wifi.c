/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: fl_ble_wifi.c
 *Created on		: Aug 20, 2025
 *Author			: Freelux RnD Team
 *Description		:
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/

#include "tl_common.h"
#include "fl_input_ext.h"
#include "fl_nwk_handler.h"
#include "../TBS_dev/TBS_dev_config.h"
#ifdef MASTER_CORE
#include "fl_ble_wifi.h"
#include "fl_nwk_protocol.h"
#include "fl_wifi2ble_fota.h"

/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
volatile fl_timetamp_withstep_t WIFI_ORIGINAL_GETALL;

#define fl_ble_send_wifi				fl_serial_send

#define BLE_WIFI_MAXLEN					(FL_TXFIFO_SIZE-3)

typedef struct {
	u8 len_data;
	u8 cmd;
	u8 crc8;
	u8 data[BLE_WIFI_MAXLEN - 3];
}__attribute__((packed)) fl_datawifi2ble_t;

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

typedef enum {
	/* For UART communication */
	GF_CMD_PING_REQUEST  = 0,
	GF_CMD_PING_RESPONSE = 0,
	GF_CMD_REPORT_REQUEST = 0x01,
	GF_CMD_REPORT_RESPONSE = 0x02,
	GF_CMD_GET_LIST_REQUEST = 0x03,
	GF_CMD_GET_LIST_RESPONSE = 0x04,
	GF_CMD_PAIRING_REQUEST = 0x05,
	GF_CMD_PAIRING_RESPONSE = 0x05,
	GF_CMD_SENDMESS_REQUEST = 0x06,
	GF_CMD_SENDMESS_RESPONSE = 0x06,
	GF_CMD_TIMESTAMP_REQUEST = 0x08,
	GF_CMD_TIMESTAMP_RESPONSE = 0x08,
	GF_CMD_DEBUG_RESPONSE =0x09,
	GF_CMD_RSTFACTORY_REQUEST = 0x0A,
	GF_CMD_RSTFACTORY_RESPONSE = 0x0A,
	GF_CMD_RSTPWMETER_REQUEST = 0x0D,
	GF_CMD_RSTPWMETER_RESPONSE = 0x0D,
	GF_CMD_PWMETER_RUNNING_SET_REQUEST = 0x0E,
	GF_CMD_PWMETER_RUNNING_SET_RESPONSE = 0x0E,
	///FOTA cmd
	GF_CMD_FOTA_REQUEST = 0xA0,
	GF_CMD_FOTA_RESPONSE = 0xA0,
}fl_wifi_cmd_e;

typedef void (*RspFunc)(u8*);
typedef struct {
	struct {
		fl_wifi_cmd_e cmd;
		void (*ReqFunc)(u8*, RspFunc fnc);
	} req;
	struct {
		fl_wifi_cmd_e cmd;
		RspFunc Rspfnc;
	} rsp;
}__attribute__((packed)) fl_wifiprotocol_proc_t;
//** Format data CMD from WIFI **//
typedef struct {
	u8 mac[6];
	u8 timetamp_begin[4];
	u8 timetamp_end[4];
} fl_wf_report_frame_t;
typedef union {
	fl_wf_report_frame_t frame;
	u8 bytes[SIZEU8(fl_wf_report_frame_t)];
}__attribute__((packed)) fl_wf_report_u;

/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
void fl_wifi2ble_Excute(fl_wifi2ble_exc_e cmd);
void PING_REQ(u8* _pdata, RspFunc rspfnc);
void PING_RSP(u8* _pdata);
void REPORT_REQUEST(u8* _pdata, RspFunc rspfnc);
void REPORT_RESPONSE(u8* _pdata);
void GETLIST_REQUEST(u8* _pdata, RspFunc rspfnc);
void GETLIST_RESPONSE(u8* _pdata);
void PAIRING_REQUEST(u8* _pdata, RspFunc rspfnc);
void PAIRING_RESPONSE(u8* _pdata){};
void TIMETAMP_REQUEST(u8* _pdata, RspFunc rspfnc);
void TIMETAMP_RESPONSE(u8* _pdata);
void SENDMESS_REQUEST(u8* _pdata, RspFunc rspfnc);
void SENDMESS_RESPONSE(u8* _pdata);
void RSTFACTORY_REQUEST(u8* _pdata, RspFunc rspfnc);
void RSTFACTORY_RESPONSE(u8* _pdata);
void RSTPWMETER_REQUEST(u8* _pdata, RspFunc rspfnc);
void RSTPWMETER_RESPONSE(u8* _pdata);
void PWMETER_RUNNING_SET_REQUEST(u8* _pdata, RspFunc rspfnc);
void PWMETER_RUNNING_SET_RESPONSE(u8* _pdata){};
//FOTA
void FOTA_REQUEST(u8* _pdata, RspFunc rspfnc);
void FOTA_RESPONSE(u8* _pdata);

fl_wifiprotocol_proc_t G_WIFI_CON[] = {
			{ { GF_CMD_PING_REQUEST, PING_REQ }, { GF_CMD_PAIRING_RESPONSE, PING_RSP } }, //ping
			{ { GF_CMD_REPORT_REQUEST, REPORT_REQUEST }, { GF_CMD_REPORT_RESPONSE, REPORT_RESPONSE } },
			{ { GF_CMD_GET_LIST_REQUEST, GETLIST_REQUEST },{GF_CMD_GET_LIST_RESPONSE, GETLIST_RESPONSE } },
			{ { GF_CMD_PAIRING_REQUEST, PAIRING_REQUEST }, {GF_CMD_PAIRING_RESPONSE, PAIRING_RESPONSE } },
			{ { GF_CMD_SENDMESS_REQUEST, SENDMESS_REQUEST }, {GF_CMD_SENDMESS_RESPONSE, SENDMESS_RESPONSE } },
			{ { GF_CMD_TIMESTAMP_REQUEST, TIMETAMP_REQUEST }, {GF_CMD_TIMESTAMP_RESPONSE, TIMETAMP_RESPONSE } },
			{ { GF_CMD_RSTFACTORY_REQUEST, RSTFACTORY_REQUEST }, {GF_CMD_RSTFACTORY_RESPONSE, RSTFACTORY_RESPONSE } },
			{ { GF_CMD_RSTPWMETER_REQUEST, RSTPWMETER_REQUEST }, {GF_CMD_RSTPWMETER_RESPONSE, RSTPWMETER_RESPONSE } },
			{ { GF_CMD_PWMETER_RUNNING_SET_REQUEST, PWMETER_RUNNING_SET_REQUEST }, {GF_CMD_PWMETER_RUNNING_SET_RESPONSE, PWMETER_RUNNING_SET_RESPONSE } },
			//FOTA
			{ { GF_CMD_FOTA_REQUEST, FOTA_REQUEST }, { GF_CMD_FOTA_RESPONSE, FOTA_RESPONSE } },
};

#define GWIFI_SIZE 				(sizeof(G_WIFI_CON)/sizeof(G_WIFI_CON[0]))

u8 _wf_CMD_find(fl_wifi_cmd_e _cmdid) {
//	LOGA(MCU,"Size CON:%d\r\n",GWIFI_SIZE);
	for (u8 var = 0; var < GWIFI_SIZE; ++var) {
		if (G_WIFI_CON[var].req.cmd == _cmdid || G_WIFI_CON[var].rsp.cmd == _cmdid) {
			return var;
		}
	}
	return 0xFF;
}

/******************************************************************************/
/******************************************************************************/
/***                      Processing functions 					             **/
/******************************************************************************/
/******************************************************************************/

static void _getnsend_data_report(u8 var, u8 rspcmd) {
	extern fl_slaves_list_t G_NODE_LIST;
	//for COUTER DEVICEs
	/* COUNTER DEVICE
	 * |Call butt|End call butt|Reset button|Pass product|Error product|  Mode |Reserve| (sum 22 bytes)
	 * |   1B	 |      1B     |     1B     |    4Bs     |     4Bs     |  1B   |  10Bs |
	 * */
	u8 payload[BLE_WIFI_MAXLEN];
	memset(payload,0xFF,SIZEU8(payload));
	fl_datawifi2ble_t wfdata;
	if (G_NODE_LIST.sla_info[var].dev_type == TBS_COUNTER) {
		tbs_device_counter_t *counter_data = (tbs_device_counter_t*) G_NODE_LIST.sla_info[var].data;
		memcpy(counter_data->mac,G_NODE_LIST.sla_info[var].mac,6);
		wfdata.cmd = rspcmd;
		wfdata.len_data = SIZEU8(tbs_device_counter_t)+ 1; //+ status
		wfdata.data[0] = G_NODE_LIST.sla_info[var].active;
		memcpy(&wfdata.data[1],(u8*)counter_data,wfdata.len_data);
		wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
		u8 len_payload = wfdata.len_data + SIZEU8(wfdata.cmd) + SIZEU8(wfdata.crc8) + SIZEU8(wfdata.len_data);
		memcpy(payload,(u8*) &wfdata,len_payload);
		P_PRINTFHEX_A(MCU,payload,len_payload,"Couter struct(%d):",len_payload);
		fl_ble_send_wifi(payload,len_payload);
	}
	//For POWER-METER DEVICEs
	/*
	 * | Frequency | Voltage | Current 1 | Current 2 | Current 3 | Power 1 | Power 2 | Power 3 | Energy 1 | Energy 2 | Energy 3 | Reserve | (sum 176 bits)
	 * |   7 bits  |  9 bits |  10 bits  |  10 bits  |  10 bits  | 14 bits | 14 bits | 14 bits | 24 bits  | 24 bits  | 24 bits  | 16 bits |
	 */
	else {
		if (G_NODE_LIST.sla_info[var].dev_type == TBS_POWERMETER) {
			wfdata.cmd = rspcmd;
			wfdata.len_data = (POWER_METER_BITSIZE-1) + 1;//+ status
			wfdata.data[0]=G_NODE_LIST.sla_info[var].active;
			memcpy(&wfdata.data[1],G_NODE_LIST.sla_info[var].data,wfdata.len_data);
			wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
			u8 len_payload = wfdata.len_data + SIZEU8(wfdata.cmd) + SIZEU8(wfdata.crc8) + SIZEU8(wfdata.len_data);
			memcpy(payload,(u8*) &wfdata,len_payload);
			//For testing parsing
			tbs_device_powermeter_t received;
			tbs_unpack_powermeter_data(&received, G_NODE_LIST.sla_info[var].data);
			tbs_power_meter_printf(MCU,(void*)&received);
			P_PRINTFHEX_A(MCU,payload,len_payload,"PW Meter struct(%d):",len_payload);
			/*Send to WIFI*/
			fl_ble_send_wifi(payload,len_payload);
		}
	}
}

void REPORT_REQUEST(u8* _pdata, RspFunc rspfnc) {
	extern fl_slaves_list_t G_NODE_LIST;
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);

	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	if (rspfnc != 0) {
		rspfnc(_pdata);
		fl_wf_report_u report_fmt;
		memcpy(report_fmt.bytes,data->data,data->len_data);
		if (IS_MAC_INVALID(report_fmt.frame.mac,0xFF) && G_NODE_LIST.slot_inused != 0xFF) {
//			fl_wifi2ble_Excute(W2B_START_NWK);
			WIFI_ORIGINAL_GETALL = fl_rtc_getWithMilliStep();
		}
	}
}

void REPORT_RESPONSE(u8* _pdata) {
	extern fl_slaves_list_t G_NODE_LIST;
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	fl_wf_report_u report_fmt;
	memcpy(report_fmt.bytes,data->data,data->len_data);

	if (IS_MAC_INVALID(report_fmt.frame.mac,0xFF)) {
		LOG_P(MCU,"Send all nodelist!!!\r\n");
		fl_nwk_master_StatusNodesRefesh();
		//todo : create array data of the all nodelist
		for (u8 var = 0; var < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; ++var) {
//			LOGA(MCU,"Devtype:%d\r\n",G_NODE_LIST.sla_info[var].dev_type);
			//addnew: only send offline nodes bcs the online nodes has automatically sent yet
			if (G_NODE_LIST.sla_info[var].active == false) {
				_getnsend_data_report(var,G_WIFI_CON[_wf_CMD_find(data->cmd)].rsp.cmd);
			}
		}
	} else {
		//todo: send data of the special nodes
		u8 slave_idx = fl_master_SlaveID_get(report_fmt.frame.mac);
		if (slave_idx != 0xFF && !memcmp(report_fmt.frame.timetamp_begin,report_fmt.frame.timetamp_end,4) &&
				MAKE_U32(report_fmt.frame.timetamp_begin[0],report_fmt.frame.timetamp_begin[1],report_fmt.frame.timetamp_begin[2],report_fmt.frame.timetamp_begin[3]) == 0) {
			_getnsend_data_report(slave_idx,G_WIFI_CON[_wf_CMD_find(data->cmd)].rsp.cmd);
		}
		//todo: get history from the flash
		else
		{
//			u16 from_index = MAKE_U16(report_fmt.frame.timetamp_begin[2],report_fmt.frame.timetamp_begin[3]);
//			u16 to_index = MAKE_U16(report_fmt.frame.timetamp_end[2],report_fmt.frame.timetamp_end[3]);
			if(-1==fl_api_master_req(report_fmt.frame.mac,NWK_HDR_A5_HIS,report_fmt.bytes,SIZEU8(report_fmt.bytes),0,0,0)){
				ERR(MCU,"REQ API !!!!\r\n");
			}
		}
	}
}
void GETLIST_REQUEST(u8* _pdata, RspFunc rspfnc) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	if (rspfnc != 0) {
		rspfnc(_pdata);
	}
}
void GETLIST_RESPONSE(u8* _pdata) {
	extern fl_slaves_list_t G_NODE_LIST;
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	fl_datawifi2ble_t wfdata;
	wfdata.cmd = G_WIFI_CON[_wf_CMD_find(data->cmd)].rsp.cmd;
	//u8 payload[BLE_WIFI_MAXLEN];
	//memset(payload,0xFF,SIZEU8(payload));
	fl_nwk_master_StatusNodesRefesh();
	u8 payload_len = 0;
	for (u8 var = 0; var < G_NODE_LIST.slot_inused && G_NODE_LIST.slot_inused != 0xFF; ++var) {
		wfdata.data[payload_len] = G_NODE_LIST.slot_inused ;
		memcpy(&wfdata.data[++payload_len],G_NODE_LIST.sla_info[var].mac,SIZEU8(G_NODE_LIST.sla_info[var].mac));
		payload_len+=SIZEU8(G_NODE_LIST.sla_info[var].mac);
		wfdata.data[payload_len] =  G_NODE_LIST.sla_info[var].dev_type;
		payload_len++;
		wfdata.data[payload_len] =  G_NODE_LIST.sla_info[var].active;
		payload_len++;
		wfdata.len_data = payload_len;
		wfdata.crc8 = fl_crc8(wfdata.data,payload_len);
		payload_len += SIZEU8(wfdata.cmd)+SIZEU8(wfdata.crc8)+SIZEU8(wfdata.len_data);
		//memcpy(payload,wfdata,payload_len);
		//P_PRINTFHEX_A(MCU,(u8*)&wfdata,payload_len,"List(%d):",payload_len);
		fl_ble_send_wifi((u8*)&wfdata,payload_len);
		//memset(payload,0xFF,SIZEU8(payload));
		payload_len = 0;
	}
	return;
}
void PAIRING_REQUEST(u8* _pdata, RspFunc rspfnc) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	extern volatile u8 MASTER_INSTALL_STATE;
	MASTER_INSTALL_STATE = (data->data[0]==0x01)?true:false;
	LOGA(MCU,"WiFi -> Collection mode:%d\r\n",MASTER_INSTALL_STATE);
	if (rspfnc != 0) {
		rspfnc(_pdata);
	}
}

void TIMETAMP_REQUEST(u8* _pdata, RspFunc rspfnc) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	if (rspfnc != 0) {
		u32 timetamp_wifi_set = MAKE_U32(data->data[3],data->data[2],data->data[1],data->data[0]);
		datetime_t cur_dt;
		fl_rtc_timestamp_to_datetime(timetamp_wifi_set,&cur_dt);
		LOGA(MCU,"TIME SET:%02d/%02d/%02d - %02d:%02d:%02d\r\n",cur_dt.year,cur_dt.month,cur_dt.day,cur_dt.hour,cur_dt.minute,cur_dt.second);
		//fl_rtc_sync(timetamp_wifi_set);
		fl_rtc_set(timetamp_wifi_set);
//		fl_rtc_set(timetamp_wifi_set);
//		/*todo: send heartbeat to network so synchronize timetamp*/
//		extern int fl_send_heartbeat(void);
//		fl_send_heartbeat();
		//RSP only use to req timetamp from master
		//rspfnc(_pdata);
	}
}
void TIMETAMP_RESPONSE(u8* _pdata) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	fl_datawifi2ble_t wfdata;
	wfdata.cmd = G_WIFI_CON[_wf_CMD_find(data->cmd)].rsp.cmd;
	memset(wfdata.data,0,SIZEU8(wfdata.data));
	wfdata.len_data = 0;
	wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
	u8 payload_len = wfdata.len_data + SIZEU8(wfdata.cmd)+SIZEU8(wfdata.crc8)+SIZEU8(wfdata.len_data);
	fl_ble_send_wifi((u8*)&wfdata,payload_len);
}

/***************************************************
 * @brief 		:slave RSP after sent REQ -> send rsp MAC to wifi
 *
 * @param[in] 	:none
 *
 * @return	  	:none
 *
 ***************************************************/
void _slave_rsp_callbackNrspWifiMAC(fl_wifi_cmd_e _rsp_cmdID,void *_data, void* _data2){
	fl_rsp_container_t *data = (fl_rsp_container_t*) _data;
	//rsp data
	if (data->timeout > 0) {
		LOGA(API,"RTT:%d ms\r\n",(data->timeout_set - data->timeout) / 1000);
		fl_pack_t *packet = (fl_pack_t *) _data2;
		P_PRINTFHEX_A(API,packet->data_arr,packet->length,"RSP: ");
		//Rsp to WIFI
		u8 mac[6];
		if (fl_master_SlaveMAC_get(data->rsp_check.slaveID,mac) != -1) {
			fl_datawifi2ble_t wfdata;
			wfdata.cmd = _rsp_cmdID;
			memset(wfdata.data,0,SIZEU8(wfdata.data));
			memcpy(wfdata.data,mac,SIZEU8(mac));
			wfdata.len_data = SIZEU8(mac);
			wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
			u8 payload_len = wfdata.len_data + SIZEU8(wfdata.cmd) + SIZEU8(wfdata.crc8) + SIZEU8(wfdata.len_data);
			fl_ble_send_wifi((u8*) &wfdata,payload_len);
		}
	} else {
		LOGA(API,"RTT: TIMEOUT (%d ms)\r\n",(data->timeout_set) / 1000);
	}
	LOGA(API,"cmdID  :%02X\r\n",data->rsp_check.hdr_cmdid);
	LOGA(API,"SlaveID:%d\r\n",data->rsp_check.slaveID);
	LOGA(API,"SeqTT  :%lld\r\n",data->rsp_check.seqTimetamp);
}
void _SENDMESS_slave_rsp_callback(void *_data,void* _data2){
	_slave_rsp_callbackNrspWifiMAC(G_WIFI_CON[_wf_CMD_find(GF_CMD_SENDMESS_REQUEST)].rsp.cmd,_data,_data2);
}

void SENDMESS_REQUEST(u8* _pdata, RspFunc rspfnc){
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	u8 mac[6];
	memcpy(mac,data->data,SIZEU8(mac));
	u8 message[22]; //max payload adv
	memset(message,0,SIZEU8(message));
	u8 len_mess = (data->len_data - SIZEU8(mac) > SIZEU8(message))? SIZEU8(message):data->len_data - SIZEU8(mac);
	//convert hex to dec : location index message
	data->data[SIZEU8(mac)] = data->data[SIZEU8(mac)]-0x30;
	memcpy(message,&data->data[SIZEU8(mac)],len_mess);

	fl_api_master_req(mac,NWK_HDR_F6_SENDMESS,message,len_mess,&_SENDMESS_slave_rsp_callback,0,1);
	if (rspfnc != 0) {
		//don't reponse in here => wait slave rsp or timeout
	}
}

void SENDMESS_RESPONSE(u8* _pdata){
}

void _PING_slave_rsp_callback(void *_data, void* _data2) {
	fl_rsp_container_t *data = (fl_rsp_container_t*) _data;
	u8 status = 0;
	u8 version = 0;
	//rsp data
	if (data->timeout > 0) {
		LOGA(API,"RTT:%d ms\r\n",(data->timeout_set - data->timeout) / 1000);
		fl_pack_t *packet = (fl_pack_t *) _data2;
		P_PRINTFHEX_A(API,packet->data_arr,packet->length,"RSP: ");
		status= 1;
		version = packet->data_arr[7];
	} else {
		LOGA(API,"RTT: TIMEOUT (%d ms)\r\n",(data->timeout_set) / 1000);
		status=0;
	}
	LOGA(API,"cmdID  :%02X\r\n",data->rsp_check.hdr_cmdid);
	LOGA(API,"SlaveID:%d\r\n",data->rsp_check.slaveID);
	LOGA(API,"SeqTT  :%lld\r\n",data->rsp_check.seqTimetamp);
	//Rsp to WIFI
	u8 mac[6];
	if (fl_master_SlaveMAC_get(data->rsp_check.slaveID,mac) != -1) {
		fl_datawifi2ble_t wfdata;
		wfdata.cmd = GF_CMD_PING_RESPONSE;
		memset(wfdata.data,0,SIZEU8(wfdata.data));
		memcpy(wfdata.data,mac,SIZEU8(mac));
		wfdata.data[6] = status;
		wfdata.data[7] = version;
		wfdata.len_data = SIZEU8(mac) + 1 + 1; // 1byte status + 1 byte version
		wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
		u8 payload_len = wfdata.len_data + SIZEU8(wfdata.cmd) + SIZEU8(wfdata.crc8) + SIZEU8(wfdata.len_data);
		fl_ble_send_wifi((u8*) &wfdata,payload_len);
	}
}

void PING_REQ(u8* _pdata, RspFunc rspfnc) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	u8 mac[6];
	memcpy(mac,data->data,SIZEU8(mac));
	fl_api_master_req(mac,NWK_HDR_22_PING,mac,SIZEU8(mac),_PING_slave_rsp_callback,0,1);
	if (rspfnc != 0) {
		//don't reponse in here => wait slave rsp or timeout
	}
}
void PING_RSP(u8* _pdata) {

}


void _RSTPWMETER_slave_rsp_callback(void *_data, void* _data2) {
	_slave_rsp_callbackNrspWifiMAC(G_WIFI_CON[_wf_CMD_find(GF_CMD_RSTPWMETER_REQUEST)].rsp.cmd,_data,_data2);
}

void RSTPWMETER_REQUEST(u8* _pdata, RspFunc rspfnc){
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	u8 mac[6];
	memcpy(mac,data->data,SIZEU8(mac));
	u8 message[22]; //max payload adv
	memset(message,0,SIZEU8(message));
	memcpy(message,&data->data[SIZEU8(mac)],3);
	fl_api_master_req(mac,NWK_HDR_F7_RSTPWMETER,message,3,&_RSTPWMETER_slave_rsp_callback,0,1);
	if (rspfnc != 0) {
		//don't reponse in here => wait slave rsp or timeout
	}
}
void RSTPWMETER_RESPONSE(u8* _pdata){};

void _PWMETER_SET_slave_rsp_callback(void *_data, void* _data2) {
	_slave_rsp_callbackNrspWifiMAC(G_WIFI_CON[_wf_CMD_find(GF_CMD_PWMETER_RUNNING_SET_REQUEST)].rsp.cmd,_data,_data2);
}
void PWMETER_RUNNING_SET_REQUEST(u8* _pdata, RspFunc rspfnc) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	u8 mac[6];
	memcpy(mac,data->data,SIZEU8(mac));
	u8 message[22]; //max payload adv
	memset(message,0,SIZEU8(message));
	memcpy(message,&data->data[SIZEU8(mac)],3*2);//3 channels - 2 bytes for each
	fl_api_master_req(mac,NWK_HDR_F8_PWMETER_SET,message,3*2,&_PWMETER_SET_slave_rsp_callback,0,1);
	if (rspfnc != 0) {
		//don't reponse in here => wait slave rsp or timeout
	}
}

void RSTFACTORY_REQUEST(u8* _pdata, RspFunc rspfnc){
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	u8 cmd_txt[12]={'f','a','c','t','o','r','y','r','e','s','e','t'};
	if(-1==plog_IndexOf(data->data,cmd_txt,SIZEU8(cmd_txt),data->len_data)){
		return;
	}
	ERR(APP,"Clear and reset factory.....\r\n");
	fl_db_clearAll();
//	delay_ms(1000);
	sys_reboot();
	return;
	//don'ts send rsp
	if (rspfnc != 0) {
		return;
	}
}
void RSTFACTORY_RESPONSE(u8* _pdata){}
/******************************************************************************/
/******************************************************************************/
/***                            FOTA Nwk Processor                           **/
/******************************************************************************/
/******************************************************************************/
void FOTA_REQUEST(u8* _pdata, RspFunc rspfnc) {
	extern fl_slaves_list_t G_NODE_LIST;
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if (crc8_cal != data->crc8) {
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
		return;
	}
	//callback fnc rsp
	if (rspfnc != 0) {
		rspfnc(data->data);
	}
}

void FOTA_RESPONSE(u8* _pdata){
//	Build rsp wifi
	fl_datawifi2ble_t wfdata;
	wfdata.cmd = G_WIFI_CON[_wf_CMD_find(GF_CMD_FOTA_REQUEST)].rsp.cmd;
	memset(wfdata.data,0,SIZEU8(wfdata.data));
	//DFU put to flash
	wfdata.data[0] = DFU_OTA_FW_PUT(_pdata);
	wfdata.len_data = 1; //<OK/ERR> 1 byte
	wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
	u8 payload_len = wfdata.len_data + SIZEU8(wfdata.cmd) + SIZEU8(wfdata.crc8) + SIZEU8(wfdata.len_data);
	fl_ble_send_wifi((u8*) &wfdata,payload_len);
}

/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/
void fl_ble_wifi_proc(u8* _pdata) {
	u8 len_cmd = 0;
	u8 cmd_in_data = 1;
	for (; cmd_in_data < _pdata[0]; cmd_in_data += len_cmd) {
		fl_datawifi2ble_t *data = (fl_datawifi2ble_t*) &_pdata[cmd_in_data];
		LOGA(DRV,"WIFI|cmd_in_data:%d,len_cmd:%d\r\n",cmd_in_data,len_cmd);
		for (u8 i = 0; i < GWIFI_SIZE; i++) {
			if (data->cmd == G_WIFI_CON[i].req.cmd) {
				G_WIFI_CON[i].req.ReqFunc(&_pdata[cmd_in_data - 1],G_WIFI_CON[i].rsp.Rspfnc);
				LOGA(DRV,"WIFI|CMDID:%d,CON_cmdid:%d\r\n",data->cmd,(u8 )G_WIFI_CON[i].req.cmd);
				break;
			}
		}
		len_cmd = data->len_data + SIZEU8(data->cmd) + SIZEU8(data->crc8) + SIZEU8(data->len_data);
	}
}

void fl_ble2wifi_HISTORY_SEND(u8* mac,u8* timetamp,u8* _data){
	extern fl_slaves_list_t G_NODE_LIST;
	u8 payload[BLE_WIFI_MAXLEN];
	memset(payload,0xFF,SIZEU8(payload));
	fl_datawifi2ble_t wfdata;

	wfdata.cmd = GF_CMD_REPORT_RESPONSE;
	u8 ind_node = fl_master_Node_find(mac);
	wfdata.data[0] = 2;//status => always 2 when got history
	memcpy(&wfdata.data[1],mac,6);
	memcpy(&wfdata.data[1+6],timetamp,4);
	wfdata.data[1 + 6 + 4] = G_NODE_LIST.sla_info[ind_node].dev_type;
	u8 data_len = 0;
	if (G_NODE_LIST.sla_info[ind_node].dev_type == TBS_COUNTER) {
		wfdata.len_data = SIZEU8(tbs_device_counter_t) + 1; //+ status
		data_len=COUNTER_DATA_INSTRUCT;
	}
	else if(G_NODE_LIST.sla_info[ind_node].dev_type == TBS_POWERMETER) {
		data_len=POWER_DATA_INSTRUCT;
		wfdata.len_data = POWER_METER_BITSIZE +1;//SIZEU8(tbs_device_powermeter_t) + 1; //+ status
	}
	memcpy(&wfdata.data[1+6+4+1],_data,data_len);

	wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
	u8 len_payload = wfdata.len_data + SIZEU8(wfdata.cmd) + SIZEU8(wfdata.crc8) + SIZEU8(wfdata.len_data);
	memcpy(payload,(u8*) &wfdata,len_payload);
	P_PRINTFHEX_A(MCU,payload,len_payload,"His %s(%d):",(G_NODE_LIST.sla_info[ind_node].dev_type == TBS_COUNTER)?"COunter":"PwMeter",len_payload);
	fl_ble_send_wifi(payload,len_payload);
}
void fl_ble2wifi_EVENT_SEND(u8* _slave_mac){
	fl_datawifi2ble_t wfdata;
	wfdata.cmd = GF_CMD_REPORT_REQUEST;
	memset(wfdata.data,0,SIZEU8(wfdata.data));
	memcpy(wfdata.data,_slave_mac,6);
	wfdata.len_data = 6 + 4 + 4; // mac + timetamp_begin + timetamp_end
	wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
	LOGA(MCU,"Ext-Call:0x%02X%02X%02X%02X%02X%02X\r\n",wfdata.data[0],wfdata.data[1],wfdata.data[2],wfdata.data[3],wfdata.data[4],wfdata.data[5]);
	u8 cmd_data[30];
	memset(cmd_data,0,SIZEU8(cmd_data));
	cmd_data[0] = wfdata.len_data + SIZEU8(wfdata.cmd)+SIZEU8(wfdata.crc8)+SIZEU8(wfdata.len_data);
	memcpy(&cmd_data[1],(u8*)&wfdata,cmd_data[0]);
	REPORT_RESPONSE(cmd_data);
}
void fl_ble2wifi_send_FOTA_BROADCAST_RSP(u8 *_rslt,u8 _size){
	fl_datawifi2ble_t wfdata;
	wfdata.cmd = GF_CMD_FOTA_RESPONSE;
	memset(wfdata.data,0,SIZEU8(wfdata.data));
	memcpy(wfdata.data,_rslt,_size);
	wfdata.len_data = _size;
	wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
	P_INFO_HEX(wfdata,wfdata.len_data+3,"FOTA RSP(%d):",wfdata.len_data+3);
	fl_ble_send_wifi((u8*)&wfdata,wfdata.len_data+3);//len_data + id_cmd + crc
}
void fl_ble2wifi_DEBUG2MQTT(u8* _payload,u8 _size){
	fl_datawifi2ble_t wfdata;
	wfdata.cmd = GF_CMD_DEBUG_RESPONSE;
	memset(wfdata.data,0,SIZEU8(wfdata.data));
	memcpy(wfdata.data,_payload,_size);
	wfdata.len_data = _size;
	wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
	P_PRINTFHEX_A(MCU,wfdata,wfdata.len_data+3,"DEBUG MQTT(%d):",wfdata.len_data+3);
	fl_ble_send_wifi((u8*)&wfdata,wfdata.len_data+3);//len_data + id_cmd + crc
}

void fl_wifi2ble_Excute(fl_wifi2ble_exc_e cmd) {
	extern fl_slaves_list_t G_NODE_LIST;
//	extern fl_adv_settings_t G_ADV_SETTINGS;
	extern void fl_nwk_protcol_ExtCall(type_debug_t _type, u8 *_data);
	type_debug_t cmd_type = SETCMD;
	char cmd_fmt[UART_DATA_LEN];
	memset(cmd_fmt,0,SIZEU8(cmd));
	switch (cmd) {
		case W2B_START_NWK: {
			if(G_NODE_LIST.slot_inused == 0xFF){break;}
			//p get info 255 <Period get again> <num slave for each> <num virtual slave> <timeout rsp> <retry cnt>
			cmd_type = GETCMD;
			//sprintf(cmd_fmt,"p get info %d %d %d %d %d %d",255,0,8,G_NODE_LIST.slot_inused,G_ADV_SETTINGS.time_wait_rsp,G_ADV_SETTINGS.retry_times);
			sprintf(cmd_fmt,"p get all 0");
			break;
		}
		default:
		break;
	}
	if(cmd_fmt[0] == 'p') fl_nwk_protcol_ExtCall(cmd_type,(u8*)cmd_fmt);
}

#endif

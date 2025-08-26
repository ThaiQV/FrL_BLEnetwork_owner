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
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/

#define fl_ble_send_wifi				fl_serial_send

#define BLE_WIFI_MAXLEN					(FL_TXFIFO_SIZE-3)
typedef struct {
	u8 len_data;
	u8 cmd;
	u8 crc8;
	u8 data[BLE_WIFI_MAXLEN-3];
}__attribute__((packed)) fl_datawifi2ble_t;

/******************************************************************************/
/******************************************************************************/
/***                           Private definitions                           **/
/******************************************************************************/
/******************************************************************************/

typedef enum{
	/* For UART communication */
	GF_CMD_PING = 0,
	GF_CMD_REPORT_REQUEST,
	GF_CMD_REPORT_RESPONSE,
	GF_CMD_GET_LIST_REQUEST,
	GF_CMD_GET_LIST_RESPONSE,
	GF_CMD_TIMESTAMP_REQUEST,
	GF_CMD_TIMESTAMP_RESPONSE,
}__attribute__((packed)) fl_wifi_cmd_e;

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

void PING_REQ(u8* _pdata,RspFunc rspfnc );
void PING_RSP(u8* _pdata);
void REPORT_REQUEST(u8* _pdata,RspFunc rspfnc );
void REPORT_RESPONSE(u8* _pdata);

fl_wifiprotocol_proc_t G_WIFI_CON[]={
		{{GF_CMD_PING,PING_REQ},{GF_CMD_PING,PING_RSP}},//ping
		{{GF_CMD_REPORT_REQUEST,REPORT_REQUEST},{GF_CMD_REPORT_RESPONSE,REPORT_RESPONSE}},
//		{GF_CMD_GET_LIST_REQUEST,},
//		{GF_CMD_GET_LIST_RESPONSE,},
//		{GF_CMD_TIMESTAMP_REQUEST,},
//		{GF_CMD_TIMESTAMP_RESPONSE,},
};
#define GWIFI_SIZE 				(sizeof(G_WIFI_CON)/sizeof(G_WIFI_CON[0]))

u8 _wf_CMD_find(fl_wifi_cmd_e _cmdid){
//	LOGA(MCU,"Size CON:%d\r\n",GWIFI_SIZE);
	for (u8 var = 0; var < GWIFI_SIZE; ++var) {
		if(G_WIFI_CON[var].req.cmd == _cmdid || G_WIFI_CON[var].rsp.cmd == _cmdid){
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

void PING_REQ(u8* _pdata,RspFunc rspfnc ){
	//fl_datawifi2ble_t *data = (fl_datawifi2ble_t*)&_pdata[1];

}
void PING_RSP(u8* _pdata){

}
void REPORT_REQUEST(u8* _pdata,RspFunc rspfnc ){
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*)&_pdata[1];
	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	u8 crc8_cal = fl_crc8(data->data,data->len_data);
	if(crc8_cal != data->crc8){
		ERR(MCU,"ERR >> CRC8:0x%02X | 0x%02X\r\n",data->crc8,crc8_cal);
	}
	if (rspfnc != 0) {
		rspfnc(_pdata);
	}
}
void REPORT_RESPONSE(u8* _pdata){
	extern fl_slaves_list_t G_NODE_LIST;
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*)&_pdata[1];
//	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
//	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
//	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
//	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");
	fl_wf_report_u report_fmt;
	memcpy(report_fmt.bytes,data->data,data->len_data);
	if (IS_MAC_INVALID(report_fmt.frame.mac,0xFF)) {
		LOG_P(MCU,"Send all nodelist!!!\r\n");
		//todo : create array data of the all nodelist

		/* COUNTER DEVICE
		 * |Call butt|End call butt|Reset button|Pass product|Error product|Reserve| (sum 22 bytes)
		 * |   1B	 |      1B     |     1B     |    4Bs     |     4Bs     |  11Bs |
		 * */
		u8 payload[BLE_WIFI_MAXLEN];
		memset(payload,0xFF,SIZEU8(payload));
		fl_datawifi2ble_t wfdata;
		for (u8 var = 0; var < G_NODE_LIST.slot_inused; ++var) {
			LOGA(MCU,"Devtype:%d\r\n",G_NODE_LIST.sla_info[var].dev_type);
			if (G_NODE_LIST.sla_info[var].dev_type == TBS_COUNTER) {
				fl_device_counter_t *counter_data = (fl_device_counter_t*) G_NODE_LIST.sla_info[var].data;
				memcpy(counter_data->data.mac,G_NODE_LIST.sla_info[var].mac,6);
				wfdata.cmd = G_WIFI_CON[_wf_CMD_find(data->cmd)].rsp.cmd;
				wfdata.len_data = SIZEU8(counter_data->bytes);
				memcpy(wfdata.data,counter_data->bytes,wfdata.len_data);
				wfdata.crc8 = fl_crc8(wfdata.data,wfdata.len_data);
				u8 len_payload = wfdata.len_data+ SIZEU8(wfdata.cmd )+ SIZEU8(wfdata.crc8)+SIZEU8(wfdata.len_data);
				memcpy(payload,(u8*)&wfdata,len_payload);
				P_PRINTFHEX_A(MCU,counter_data->bytes,sizeof(counter_data->bytes),"Couter struct(%d):",len_payload);
				fl_ble_send_wifi(payload,len_payload);
			}
		}
	} else {
		//todo: send history of the special nodes
	}

}


/******************************************************************************/
/******************************************************************************/
/***                            Functions callback                           **/
/******************************************************************************/
/******************************************************************************/

void fl_ble_wifi_proc(u8* _pdata) {
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*)&_pdata[1];
	for(u8 i=0;i<GWIFI_SIZE;i++){
		if(data->cmd == G_WIFI_CON[i].req.cmd){
			G_WIFI_CON[i].req.ReqFunc(_pdata,G_WIFI_CON[i].rsp.Rspfnc);
		}
	}
}

#endif

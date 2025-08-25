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
#ifdef MASTER_CORE
/******************************************************************************/
/******************************************************************************/
/***                                Global Parameters                        **/
/******************************************************************************/
/******************************************************************************/
#define fl_ble_send_wifi				fl_serial_send

#define BLE_WIFI_MAXLEN					22
typedef struct {
	u8 len_data;
	u8 cmd;
	u8 crc8;
	u8 data[BLE_WIFI_MAXLEN];
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
	if(fl_crc8(data->data,data->len_data) != data->crc8){
		ERR(MCU,"ERR >> CRC8:0x%02X\r\n",data->crc8);
	}
	if (rspfnc != 0) {
		fl_wf_report_u report_fmt;
		memcpy(report_fmt.bytes,data->data,data->len_data);
		if (IS_MAC_INVALID(report_fmt.frame.mac,0xFF)) {
			LOG_P(MCU,"Send all nodelist!!!\r\n");
			//todo : create array data of the all nodelist

		} else {
			//todo: send history of the special nodes
		}
		rspfnc(report_fmt.bytes);
	}
}

void REPORT_RESPONSE(u8* _pdata){
	fl_datawifi2ble_t *data = (fl_datawifi2ble_t*)&_pdata[1];
//	LOGA(MCU,"LEN:0x%02X\r\n",data->len_data);
//	LOGA(MCU,"cmdID:0x%02X\r\n",data->cmd);
//	LOGA(MCU,"CRC8:0x%02X\r\n",data->crc8);
//	P_PRINTFHEX_A(MCU,data->data,data->len_data,"Data:");

/* COUNTER DEVICE
 * |Call butt|End call butt|Reset button|Pass product|Error product|Reserve| (sum 22 bytes)
 * |   1B	 |      1B     |     1B     |    4Bs     |     4Bs     |  11Bs |
 * */

	fl_ble_send_wifi(data->data,data->len_data);
}
/******************************************************************************/
/******************************************************************************/
/***                       Functions declare                   		         **/
/******************************************************************************/
/******************************************************************************/
fl_wifiprotocol_proc_t G_WIFI_CON[]={
		{{GF_CMD_PING,PING_REQ},{GF_CMD_PING,PING_RSP}},//ping
		{{GF_CMD_REPORT_REQUEST,REPORT_REQUEST},{GF_CMD_REPORT_RESPONSE,REPORT_RESPONSE}},
//		{GF_CMD_GET_LIST_REQUEST,},
//		{GF_CMD_GET_LIST_RESPONSE,},
//		{GF_CMD_TIMESTAMP_REQUEST,},
//		{GF_CMD_TIMESTAMP_RESPONSE,},
};
#define GWIFI_SIZE 				(sizeof(G_WIFI_CON)/sizeof(G_WIFI_CON[0]))
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

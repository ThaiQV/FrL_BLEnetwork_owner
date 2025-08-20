#ifndef _NVM_H_
#define _NVM_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "../vendor/FrL_Network/fl_input_ext.h"
#include "uart.h"

/* Parameters */
#define UART_FIFO_SIZE					2048
#define UART_FRAME_SIZE					32
#define QUEUE_TIMEOUT_MAX				100

#define GF_CMD_DATA_SIZE				0xFF
#define BLE_MAC_ADDRESS_LEN				0x06
#define BLE_MAC_STRING_LEN				17

/* For UART communication */
typedef enum
{
    GF_CMD_PING = 0,
    GF_CMD_REPORT_REQUEST,
    GF_CMD_REPORT_RESPONSE,
    GF_CMD_GET_LIST_REQUEST,
    GF_CMD_GET_LIST_RESPONSE,
    GF_CMD_TIMESTAMP_REQUEST,
    GF_CMD_TIMESTAMP_RESPONSE,
}gf_command_type_t;

typedef struct
{
   uint8_t              len;
   gf_command_type_t    cmd;
   uint8_t              crc;
   uint8_t              data[GF_CMD_DATA_SIZE];
}gf_command_t;

typedef enum
{
    GF_RET_OK = 0,
    GF_RET_ERROR
}gf_ret_t;

typedef void (*gf_cmd_callback)(uint8_t *pdata, uint8_t len);

void ble_wifi_protocol_init(void);
int ble_wifi_protocol_process(void);
void ble_wifi_protocol_put_queue(uint8_t *pdata, uint8_t len);
void uart_queue_reset(void);

uint8_t gf_cmd_crc_calculate(uint8_t *pdata, uint8_t len);
void gf_command_process(gf_command_t *cmd);
void gf_cmd_send(uint8_t len, uint8_t cmd, uint8_t crc,uint8_t *data);
void gf_cmd_ping(uint8_t *pdata, uint8_t len);
void gf_cmd_report_request(uint8_t *pdata, uint8_t len);
void gf_cmd_report_response(uint8_t *pdata, uint8_t len);
void gf_cmd_getlist_request(uint8_t *pdata, uint8_t len);
void gf_cmd_getlist_response(uint8_t *pdata, uint8_t len);
void gf_cmd_timestamp_request(uint8_t *pdata, uint8_t len);
void gf_cmd_timestamp_response(uint8_t *pdata, uint8_t len);
#endif

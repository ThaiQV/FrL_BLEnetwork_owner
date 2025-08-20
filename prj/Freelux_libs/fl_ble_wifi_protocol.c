/**
  * @file       nvm.c
  * @author     LVD
  * @date       01/10/24
  * @version    V1.0.0
  * @brief     	API for nvm
  */
#include "fl_ble_wifi_protocol.h"
#include "os_queue.h"

/*------------Definitions------------*/
#define	BLE_WIFI_LOG_ENABLE		1
#if BLE_WIFI_LOG_ENABLE
#define BW_LOG(...)	LOGA(DRV,__VA_ARGS__)
#else
#define BW_LOG(...)
#endif

/* Variables */
queue_t		queue_uart;
uint32_t	queue_timeout = 0;
uint8_t 	uart_fifo[UART_FIFO_SIZE];
gf_cmd_callback	callback_funcs[] =
{
	gf_cmd_ping,
	gf_cmd_report_request,
	gf_cmd_report_response,
	gf_cmd_getlist_request,
	gf_cmd_getlist_response,
	gf_cmd_timestamp_request,
	gf_cmd_timestamp_response,
};
/* Functions */

/**
* @brief: Init BLE WIFI UART protocol
* @param: see below
* @retval: None
*/
void ble_wifi_protocol_init(void)
{
	if (blt_soft_timer_find(&ble_wifi_protocol_process) < 0)
	{
		blt_soft_timer_add(&ble_wifi_protocol_process,10 * 1000);
		queue_create(&queue_uart,uart_fifo,UART_FIFO_SIZE);
		BW_LOG("ble_wifi_protocol_init\n");
	}
}

/**
* @brief: BLE WIFI UART process
* @param: see below
* @retval: None
*/
int ble_wifi_protocol_process(void)
{
	gf_command_t			cmd;
	uint32_t 				queue_len = 0;
	uint8_t					data_len = 0;
	uint8_t					get_data;

	queue_ret_t				retval = QUEUE_RET_OK;

	queue_len = queue_available_data(&queue_uart);
	if( queue_len > 0)
	{
		data_len = queue_peek(&queue_uart);
		if (queue_len >= (data_len + 3)) // len of data + len + cmd + crc
		{
			retval += queue_get(&queue_uart,(uint8_t*)&get_data,1);
			cmd.len = get_data;
			retval += queue_get(&queue_uart,(uint8_t*)&get_data,1);
			cmd.cmd = get_data;
			retval += queue_get(&queue_uart,(uint8_t*)&get_data,1);
			cmd.crc = get_data;
			if(data_len > 0)
			{
				retval += queue_get(&queue_uart,(uint8_t*)&cmd.data,data_len);
			}
			if(retval != QUEUE_RET_OK)
			{
				uart_queue_reset();
				return 0;
			}
			gf_command_process(&cmd);
			// Refresh queue_timeout
			queue_timeout = clock_time();
		}
	}

	if((clock_time() - queue_timeout) >= QUEUE_TIMEOUT_MAX)
	{
		uart_queue_reset();
	}
	return 0;
}

void ble_wifi_protocol_put_queue(uint8_t *pdata, uint8_t len)
{
	queue_put(&queue_uart,pdata,len);
}
/**
*@brief	put data to queue
*@param see below
*/
void uart_queue_reset(void)
{
	// Re-init queue to clear queue data
	queue_create(&queue_uart,uart_fifo,UART_FIFO_SIZE);
	queue_timeout = clock_time();
}
/***********************************************Gateway BLE WIFI Commands***********************************************/
/**
*@brief	calculate CRC
*@param see below
*/
uint8_t gf_cmd_crc_calculate(uint8_t *pdata, uint8_t len)
{
	uint8_t i;
	uint8_t crc = 0;

	for (i = 0; i < len; i++)
	{
		crc += pdata[i];
	}
	return crc;
}

void gf_command_process(gf_command_t *cmd)
{
	BW_LOG("CRC: %x %x\n",gf_cmd_crc_calculate(cmd->data,cmd->len),cmd->crc);
	if(gf_cmd_crc_calculate(cmd->data,cmd->len) != cmd->crc)
	{
		return;
	}
	if(callback_funcs[cmd->cmd] != NULL) callback_funcs[cmd->cmd](cmd->data,cmd->len);
}

void gf_cmd_send(uint8_t len, uint8_t cmd, uint8_t crc,uint8_t *data)
{
	uart_send_dma(UART1,&len,sizeof(len));
	uart_send_dma(UART1,&cmd,sizeof(cmd));
	uart_send_dma(UART1,&crc,sizeof(crc));
	uart_send_dma(UART1,data,len);
}

void gf_cmd_ping(uint8_t *pdata, uint8_t len)
{
	uint8_t cmd,crc;
	uint8_t data[BLE_MAC_ADDRESS_LEN + 1]; // MAC and status

	cmd = GF_CMD_PING;
	// copy MAC address
	memcpy(data,pdata,BLE_MAC_ADDRESS_LEN);
	// status of device(offline/online)
	data[sizeof(data) - 1] = 1;
	crc = gf_cmd_crc_calculate(data,sizeof(data));
	gf_cmd_send(sizeof(data),cmd,crc,data);

	BW_LOG("Ping\n");
}

void gf_cmd_report_request(uint8_t *pdata, uint8_t len)
{
	BW_LOG("Report request\n");
}

void gf_cmd_report_response(uint8_t *pdata, uint8_t len)
{
	BW_LOG("Report response\n");
}

void gf_cmd_getlist_request(uint8_t *pdata, uint8_t len)
{
	BW_LOG("Get list request\n");
}

void gf_cmd_getlist_response(uint8_t *pdata, uint8_t len)
{
	BW_LOG("Get list response\n");
}

void gf_cmd_timestamp_request(uint8_t *pdata, uint8_t len)
{
	BW_LOG("Timestamp request\n");
}

void gf_cmd_timestamp_response(uint8_t *pdata, uint8_t len)
{
	BW_LOG("Timestamp response: %d\n");
}

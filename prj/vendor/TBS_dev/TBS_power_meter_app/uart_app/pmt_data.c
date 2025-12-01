/*
 * uart_user.c
 *
 *  Created on: Sep 14, 2023
 *      Author: hoang
 */

#include "pmt_data.h"
#include "uart_protocol.h"


uart_driver_t g_uart_driver;
static protocol_context_t g_protocol_pmt;   

static void my_uart_send(uint8_t *data, uint16_t len);
static uint32_t my_get_tick(void);
static void pmt_data_callback(uint8_t protocol_id, const uint8_t *data, uint16_t len);
static void protocol_debug_log(const char *msg);

void pmt_protocol_init()
{
    uart_driver_init(&g_uart_driver, my_uart_send, my_get_tick);
    print_uart("UART Driver initialized\n");

    protocol_init(&g_protocol_pmt, &g_uart_driver, 0x01, pmt_data_callback);
    protocol_set_debug_callback(&g_protocol_pmt, protocol_debug_log);
    print_uart("Protocol Sensor (ID=0x01) initialized\n");
}

void pmt_protocol_loop()
{
    uart_driver_process(&g_uart_driver);
    protocol_process(&g_protocol_pmt);

    static uint64_t appTimeTick = 0;
	if(get_system_time_ms() - appTimeTick > 20000){
		appTimeTick = get_system_time_ms()  ; //1ms
	}
	else{
		return ;
	}

    pmt_read_value();
}

#ifdef PMT_CLIENT
uint8_t pmt_req(uint8_t data_type)
{
    pmt_data_payload_t data;
    memset((uint8_t*)&data, 0 , sizeof(data));

    data.cmd_id = PMT_CMD_ID_RQE;
    switch (data_type)
    {
    case PMT_DATA_TYPE_CALIB:
        data.data_type = PMT_DATA_TYPE_CALIB;

        break;

    case PMT_DATA_TYPE_VOL_CUR:
        data.data_type = PMT_DATA_TYPE_VOL_CUR;

        break;
    
    default:
        break;
    }
    
}
#else
uint8_t pmt_read_value(void)
{
    uint8_t sensor_data[] = {PMT_CMD_ID_READ, PMT_DATA_TYPE_VOL_CUR};
    protocol_send(&g_protocol_pmt, sensor_data, sizeof(sensor_data));
}

uint8_t pmt_set_calib(uint8_t ch, uint16_t vol_calib, uint32_t cur_calib)
{

    pmt_data_payload_t data;
    memset((uint8_t*)&data, 0 , sizeof(data));

    data.cmd_id = PMT_CMD_ID_SET;
    data.data_type = PMT_DATA_TYPE_CALIB;
    data.value[ch].vol_calib = vol_calib;
    data.value[ch].cur_calib = cur_calib;

    protocol_send(&g_protocol_pmt, (uint8_t *)&data, sizeof(data));
}
#endif

void pmt_protocol_uart_receive(uint8_t *rx_buff, uint16_t len)
{
	uart_driver_receive_irq(&g_uart_driver, rx_buff, len);
}
/****************** Static Function ********************/
static void my_uart_send(uint8_t *data, uint16_t len) {
    drv_uart_tx_start((uint8_t *)data, len);
}

static uint32_t my_get_tick(void) {
	get_system_time_ms();
}

// Callback cho sensor protocol
static void pmt_data_callback(uint8_t protocol_id, const uint8_t *data, uint16_t len) {
    print_uart("[SENSOR] Received %d bytes: ", len);
    // for (uint16_t i = 0; i < len; i++) {
    //     print_uart("%02X ", data[i]);
    // }
    print_uart("\n");

    uint8_t cmd_id = data[0];
    uint8_t data_type = data[1];

    switch (cmd_id)
    {
    case PMT_CMD_ID_READ:
        print_uart("PMT_CMD_ID_READ\n");
        break;

    case PMT_CMD_ID_SET:
        print_uart("PMT_CMD_ID_SET\n");
        break; 
    
    default:
        break;
    }
    
    // Parse sensor data
    // ...
}

// Debug log callback
static void protocol_debug_log(const char *msg) {
    print_uart("[PROTOCOL_DEBUG] %s\n", msg);
}

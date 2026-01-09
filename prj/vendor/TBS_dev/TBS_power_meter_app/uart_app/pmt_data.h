/*
 * uart_user.h
 *
 *      Author: hoang
 */

#ifndef PMT_DATA_H_
#define PMT_DATA_H_

#ifndef MASTER_CORE
#ifdef POWER_METER_DEVICE

#include <stdint.h>
#include <stdbool.h>

// #define PMT_CLIENT
#ifndef PMT_CLIENT
#define PMT_SEVER
#endif

// data payload 
/**
 * __________________________________________________________________
 * | cmd id |  data type | time | value ch1 | value ch2 | value ch3 |
 * |--------|------------|------|-----------|-----------|-----------|
 * | 1byte  |  1 byte    |4 byte| 6 byte    | 6 byte    | 6 byte    |
 * |________|____________|______|___________|___________|___________|
 */
#define NUMBER_CHANNEL          3

//cmd id
#define PMT_CMD_ID_READ         0x01
#define PMT_CMD_ID_SET          0x02
#define PMT_CMD_ID_RQE          0x02

#define PMT_DATA_TYPE_VOL_CUR   0x01
#define PMT_DATA_TYPE_CALIB     0x01

typedef struct st_pmt_data_value
{
    uint16_t vol_calib;
    uint32_t cur_calib;
} st_pmt_data_value_t;


typedef struct pmt_data_payload
{
    uint8_t cmd_id;
    uint8_t data_type;
    st_pmt_data_value_t value[NUMBER_CHANNEL];
} pmt_data_payload_t;

void pmt_protocol_init();
void pmt_protocol_loop();

void pmt_protocol_uart_receive(uint8_t *rx_buff, uint16_t len);

#ifdef PMT_CLIENT
uint8_t pmt_req(uint8_t cmd_id);
#else
uint8_t pmt_read_value(void);
uint8_t pmt_set_calib(uint8_t ch, uint16_t vol_calib, uint32_t cur_calib);
#endif

#endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
#endif /* PMT_DATA_H_ */

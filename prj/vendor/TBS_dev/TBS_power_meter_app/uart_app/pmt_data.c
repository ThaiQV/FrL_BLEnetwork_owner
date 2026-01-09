/*
 * uart_user.c
 *
 *  Created on: Sep 14, 2023
 *      Author: hoang
 */

#include <stdio.h>
#include <string.h>

#ifndef MASTER_CORE
#include "tl_common.h"
#include "vendor/TBS_dev/TBS_dev_config.h"
#ifdef POWER_METER_DEVICE

#include "pmt_data.h"
#include "uart_protocol.h"
#include "../power_meter_app.h"

uart_driver_t g_uart_driver;
static protocol_context_t g_protocol_pmt;

static void my_uart_send(uint8_t *data, uint16_t len);
static uint32_t my_get_tick(void);
static void pmt_data_callback(uint8_t protocol_id, const uint8_t *data, uint16_t len);
static void protocol_debug_log(const char *msg);
static void process_data(const uint8_t *data, uint16_t len);

void pmt_protocol_init()
{
    uart_driver_init(&g_uart_driver, my_uart_send, my_get_tick);
    print_uart("UART Driver initialized\n");

    protocol_init(&g_protocol_pmt, &g_uart_driver, 'R', pmt_data_callback);
    protocol_set_debug_callback(&g_protocol_pmt, protocol_debug_log);
    print_uart("Protocol Sensor (ID=0x01) initialized\n");
}

void pmt_protocol_loop()
{
    uart_driver_process(&g_uart_driver);
    protocol_process(&g_protocol_pmt);

    static uint64_t appTimeTick = 0;
    if (get_system_time_ms() - appTimeTick > 20000)
    {
        appTimeTick = get_system_time_ms(); // 1ms
    }
    else
    {
        return;
    }

    // pmt_read_value();
}

#ifdef PMT_CLIENT
uint8_t pmt_req(uint8_t data_type)
{
    pmt_data_payload_t data;
    memset((uint8_t *)&data, 0, sizeof(data));

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
    memset((uint8_t *)&data, 0, sizeof(data));

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

#define MAX_TOKENS 10
#define MAX_TOKEN_LEN 50

static void my_uart_send(uint8_t *data, uint16_t len)
{
    drv_uart_tx_start((uint8_t *)data, len);
}

static uint32_t my_get_tick(void)
{
    get_system_time_ms();
}

// Callback cho sensor protocol
static void pmt_data_callback(uint8_t protocol_id, const uint8_t *data, uint16_t len)
{
    // PMT_LOGA("[SENSOR] Received %d bytes: ", len);
    // for (uint16_t i = 0; i < len; i++) {
    //     PMT_LOGA("%c", data[i]);
    // }
    // PMT_LOGA("\n");
    process_data(data, len);
    return;

    uint8_t cmd_id = data[0];
    uint8_t data_type = data[1];

    switch (cmd_id)
    {
    case PMT_CMD_ID_READ:
        PMT_LOGA("PMT_CMD_ID_READ\n");
        break;

    case PMT_CMD_ID_SET:
        PMT_LOGA("PMT_CMD_ID_SET\n");
        break;

    default:
        break;
    }

    // Parse sensor data
    // ...
}

// Debug log callback
static void protocol_debug_log(const char *msg)
{
    print_uart("[PROTOCOL_DEBUG] %s\n", msg);
}

static int split_buffer(uint8_t *buffer, uint16_t len,
                        char tokens[MAX_TOKENS][MAX_TOKEN_LEN])
{
    int token_index = 0;
    int char_index = 0;

    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t c = buffer[i];

        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
        {
            // nếu đang ghi token -> kết thúc token
            if (char_index > 0)
            {
                tokens[token_index][char_index] = '\0'; // kết thúc chuỗi
                token_index++;

                if (token_index >= MAX_TOKENS)
                    return token_index; // đầy rồi

                char_index = 0;
            }
        }
        else
        {
            if (char_index < MAX_TOKEN_LEN - 1)
            {
                tokens[token_index][char_index++] = c;
            }
        }
    }

    // kết thúc token cuối cùng nếu có
    if (char_index > 0 && token_index < MAX_TOKENS)
    {
        tokens[token_index][char_index] = '\0';
        token_index++;
    }

    return token_index; // số token
}

static float parse_float(const char *s)
{
    char buf[32];
    int j = 0;

    // Copy chuỗi vào buffer và thay ',' thành '.'
    for (int i = 0; s[i] != '\0' && j < sizeof(buf) - 1; i++)
    {
        if (s[i] == ',')
            buf[j++] = '.';
        else
            buf[j++] = s[i];
    }
    buf[j] = '\0';

    // parse thủ công (chính xác như atof)
    float sign = 1.0f;

    int idx = 0;
    if (buf[idx] == '-')
    {
        sign = -1.0f;
        idx++;
    }
    else if (buf[idx] == '+')
    {
        idx++;
    }

    float result = 0.0f;

    // Phần nguyên
    while (buf[idx] >= '0' && buf[idx] <= '9')
    {
        result = result * 10.0f + (buf[idx] - '0');
        idx++;
    }

    // Phần thập phân
    if (buf[idx] == '.')
    {
        idx++;
        float frac = 0.0f;
        float base = 0.1f;

        while (buf[idx] >= '0' && buf[idx] <= '9')
        {
            frac += (buf[idx] - '0') * base;
            base *= 0.1f;
            idx++;
        }

        result += frac;
    }

    return result * sign;
}

static void process_data(const uint8_t *data, uint16_t len)
{
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    int count = split_buffer(data, len, tokens);
    float calib_U, calib_I, calib_P;
    float U, I, P;

    // PMT_LOGA("str %d:\n", count);
    // for (int i = 0; i < count; i++) {
    //     PMT_LOGA("%s\n", tokens[i]);
    // }

    if (strcmp(tokens[0], "read") == 0)
    {
        PMT_LOGA("read: ");

        if (strcmp(tokens[1], "all") == 0)
        {
            PMT_LOGA("all\n");
            PMT_LOGA("ch1: U: %10.3f I: %10.3f P: %10.3f\n", pmt_read_U(1), pmt_read_I(1), pmt_read_P(1));
            PMT_LOGA("ch2: U: %10.3f I: %10.3f P: %10.3f\n", pmt_read_U(2), pmt_read_I(2), pmt_read_P(2));
            PMT_LOGA("ch3: U: %10.3f I: %10.3f P: %10.3f\n", pmt_read_U(3), pmt_read_I(3), pmt_read_P(3));
        }
        else if (strcmp(tokens[1], "ch1") == 0)
        {
            PMT_LOGA("ch1\n");
            PMT_LOGA("ch1: U: %5.3f (V)  -I: %5.3f (A)  -P: %5.3f (W)\n", pmt_read_U(1), pmt_read_I(1), pmt_read_P(1));
        }
        else if (strcmp(tokens[1], "ch2") == 0)
        {
            PMT_LOGA("ch2\n");
            PMT_LOGA("ch2: U: %5.3f (V)  -I: %5.3f (A)  -P: %5.3f (W)\n", pmt_read_U(2), pmt_read_I(2), pmt_read_P(2));
        }
        else if (strcmp(tokens[1], "ch3") == 0)
        {
            PMT_LOGA("ch3\n");
            PMT_LOGA("ch3: U: %5.3f (V)  -I: %5.3f (A)  -P: %5.3f (W)\n", pmt_read_U(3), pmt_read_I(3), pmt_read_P(3));
        }
        else if (strcmp(tokens[1], "calib") == 0)
        {
            PMT_LOGA("calib: ");

            if (strcmp(tokens[2], "ch1") == 0)
            {
                PMT_LOGA("ch1\n");
                pmt_getcalib(1, &calib_U, &calib_I, &calib_P);
                PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
            }
            else if (strcmp(tokens[2], "ch2") == 0)
            {
                PMT_LOGA("ch2\n");
                pmt_getcalib(2, &calib_U, &calib_I, &calib_P);
                PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
            }
            else if (strcmp(tokens[2], "ch3") == 0)
            {
                PMT_LOGA("ch3\n");
                pmt_getcalib(3, &calib_U, &calib_I, &calib_P);
                PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
            }
        }
        else if (strcmp(tokens[1], "calibr") == 0)
        {
            PMT_LOGA("calibr: ");

            if (strcmp(tokens[2], "ch1") == 0)
            {
                PMT_LOGA("ch1\n");
                pmt_getcalibr(1, &calib_U, &calib_I, &calib_P);
                PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
            }
            else if (strcmp(tokens[2], "ch2") == 0)
            {
                PMT_LOGA("ch2\n");
                pmt_getcalibr(2, &calib_U, &calib_I, &calib_P);
                PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
            }
            else if (strcmp(tokens[2], "ch3") == 0)
            {
                PMT_LOGA("ch3\n");
                pmt_getcalibr(3, &calib_U, &calib_I, &calib_P);
                PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
            }
        }
    }
    else if (strcmp(tokens[0], "setr") == 0)
    {

        calib_U = parse_float(tokens[2]);
        calib_I = parse_float(tokens[3]);
        calib_P = parse_float(tokens[4]);
        PMT_LOGA("setr: ");
        if (strcmp(tokens[1], "ch1") == 0)
        {
            PMT_LOGA("ch1\n");
            pmt_setcalibr(1, (uint16_t)calib_U, (uint16_t)calib_I, (uint16_t)calib_P);
            pmt_getcalibr(1, &calib_U, &calib_I, &calib_P);
            PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
        }
        else if (strcmp(tokens[1], "ch2") == 0)
        {
            PMT_LOGA("ch2\n");
            pmt_setcalibr(2, (uint16_t)calib_U, (uint16_t)calib_I, (uint16_t)calib_P);
            pmt_getcalibr(2, &calib_U, &calib_I, &calib_P);
            PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
        }
        else if (strcmp(tokens[1], "ch3") == 0)
        {
            PMT_LOGA("ch3\n");
            pmt_setcalibr(3, (uint16_t)calib_U, (uint16_t)calib_I, (uint16_t)calib_P);
            pmt_getcalibr(3, &calib_U, &calib_I, &calib_P);
            PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
        }
    }
    else if (strcmp(tokens[0], "set") == 0)
    {

        calib_U = parse_float(tokens[2]);
        calib_I = parse_float(tokens[3]);
        calib_P = parse_float(tokens[4]);
        PMT_LOGA("set: ");
        if (strcmp(tokens[1], "ch1") == 0)
        {
            PMT_LOGA("ch1\n");
            pmt_setcalib(1, calib_U, calib_I, calib_P);
            pmt_getcalib(1, &calib_U, &calib_I, &calib_P);
            PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
        }
        else if (strcmp(tokens[1], "ch2") == 0)
        {
            PMT_LOGA("ch2\n");
            pmt_setcalib(2, calib_U, calib_I, calib_P);
            pmt_getcalib(2, &calib_U, &calib_I, &calib_P);
            PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
        }
        else if (strcmp(tokens[1], "ch3") == 0)
        {
            PMT_LOGA("ch3\n");
            pmt_setcalib(3, calib_U, calib_I, calib_P);
            pmt_getcalib(3, &calib_U, &calib_I, &calib_P);
            PMT_LOGA("get calib: calibU: %5.3f calibI: %5.3f calibP: %5.3f\n", calib_U, calib_I, calib_P);
        }
    }
    else if (strcmp(tokens[0], "info") == 0)
    {
        PMT_LOGA("info: ");

        if (strcmp(tokens[1], "ch1") == 0)
        {
            PMT_LOGA("ch1\n");
            pmt_print_info(1);
        }
        else if (strcmp(tokens[1], "ch2") == 0)
        {
            PMT_LOGA("ch2\n");
            pmt_print_info(2);
        }
        else if (strcmp(tokens[1], "ch3") == 0)
        {
            PMT_LOGA("ch3\n");
            pmt_print_info(3);
        }
    }
}

#endif /* POWER_METER_DEVICE*/
#endif /* MASTER_CORE*/
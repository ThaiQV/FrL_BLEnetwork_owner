/*
 * mstick.c
 *
 *  Created on: Nov 30, 2023
 *      Author: hoang
 */

#ifndef MASTER_CORE

#include "tl_common.h"
#include "stdio.h"
#include "msTick.h"
#include <stdint.h>

//reg_system_tick

static uint64_t tick_ext = 0;
static uint32_t last_tick = 0;

static inline void update_tick_ext(void)
{
    uint32_t cur = reg_system_tick;
    if (cur < last_tick) {
        // detect overflow
        tick_ext += (1ULL << 32);
    }
    last_tick = cur;
}

uint64_t get_system_time_us(void)
{
    update_tick_ext();

    uint64_t tick64 = tick_ext | last_tick;

    return tick64 / SYSTEM_TIMER_TICK_1US;
}

uint64_t get_system_time_ms(void)
{
    update_tick_ext();

    uint64_t tick64 = tick_ext | last_tick;

    return tick64 / SYSTEM_TIMER_TICK_1MS;
}

#endif /* MASTER_CORE*/
/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "custom_board.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "app_scheduler.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "ble_stack.h"
#include "ble_comm.h"
#include "notification_buffers.h"
#include "nrf_pt.h"
#include "sys_time.h"
#include "tim1_sharing.h"
#include "adc_module.h" 
#include "app_logic.h"
#include "dac.h"

#include "log_config.h"
#define NRF_LOG_MODULE_NAME "main"
#define NRF_LOG_LEVEL LOG_CFG_MAIN_LOG_LEVEL
#include "nrf_log.h"
#include "nrf_log_ctrl.h"


/**@brief Function for placing the application in low power state while waiting for events.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

#define Q_NBR 3U
NOTIF_BUFFERS_DEF_QUEUES(Q_NBR, free_q, inuse_q);
uint8_t mem_pool[Q_NBR][NOTIF_BUFFERS_ONE_BUFFER_LEN];

PT_THREAD(test_blecomm(pt_t* pt)) {
    static uint8_t data[21] = {0};
    static uint32_t err = NRF_SUCCESS;
    PT_BEGIN(pt);
    UNUSED_VARIABLE(PT_YIELD_FLAG);

    for (int i=0; i < sizeof(data); ++i) {
        data[i] = (uint8_t)data[i]+i;
    }

    PT_WAIT_UNTIL(pt, true == BLE_COMM_is_comm_enabled());

    do {
        err = BLE_COMM_send_data(data, sizeof(data));
//        PT_WAIT_UNTIL(pt, NRF_SUCCESS == BLE_COMM_send_data(data, sizeof(data)));

    } while (NRF_SUCCESS == err);

//    NRF_LOG_DEBUG("Restarting pt\n");
    PT_RESTART(pt);


    PT_END(pt);
}

PT_THREAD(test_sys_tim(pt_t* pt)) {
    static uint32_t start_tp = 0U;
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, (SYS_TIME_get_tick() - start_tp) < 1000U);

    start_tp = SYS_TIME_get_tick();
    NRF_LOG_DEBUG("1s elapsed, ticks %u\n", start_tp);

    PT_RESTART(pt);

    PT_END(pt);
}

void mytest(void) {
    static uint8_t data[21] = {0};
    if (true == BLE_COMM_is_comm_enabled()) {
        for (int i=0; i < sizeof(data); ++i) {
            data[i] = (uint8_t)data[i]+i;
        }

        while (NRF_SUCCESS == BLE_COMM_send_data(data, sizeof(data)));

    }
}

int main(void) {
    // Set the external high frequency clock source to 32 MHz
    NRF_CLOCK->XTALFREQ = 0xFFFFFF00;

    // Initialize.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
    APP_SCHED_INIT(6, 10);

    NRF_LOG_INIT(NULL);
    TIM1_SHARING_init();

    SYS_TIME_init();

    ADC_init_t adc_init = {
            .samples_callback = APP_LOGIC_adc_samples_callback
    };
    if (false == ADC_init(&adc_init)) {
        NRF_LOG_ERROR("Failed to initialize ADC\n");
    }


    BLE_STACK_init();

    if (false == BLE_COMM_init()) {
        NRF_LOG_ERROR("Failed to init BLE COMM\n");
    }

    if (false == DAC_init()) {
        NRF_LOG_ERROR("Failed to init DAC\n");
    } else {
        if (false == DAC_write_vol_sett_blocking()) {
            NRF_LOG_ERROR("Failed to write settings in nvm\n");
        }

        if (false == DAC_write_vol_dac_blocking(410)) {
            NRF_LOG_ERROR("Failed to write volatile dac\n");
        }
        // Do read if device is present
        MCP47x6_read_t read_data = {{0}};
        if (true == DAC_read_blocking(&read_data)) {
            NRF_LOG_PROCESS();
            NRF_LOG_DEBUG("Successfully read from DAC, gain is %d, ref is %d, dac is %u\n", read_data.vol_sett.gain, read_data.vol_sett.vref,
                    read_data.vol_data);
            NRF_LOG_PROCESS();

        } else {
            NRF_LOG_ERROR("Failed to read from DAC\n");
        }
    }

    NRF_LOG_INFO("App started\n");


    uint32_t err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

    NOTIF_BUFFERS_descr_t descr = {0};
    const NOTIF_BUFFERS_init_t init = {
            .free_buffers_queue = &free_q,
            .in_use_buffers_queue = &inuse_q,
            .buffers_count = Q_NBR,
            .mem_pool = mem_pool
    };
    while(NRF_LOG_PROCESS());

    if (false == NOTIF_BUFFERS_init(&init, &descr)) {
        NRF_LOG_ERROR("Failed to init notif buff\r\n");
    } else {
        NRF_LOG_INFO("NOTIF BUFF initialized\n");
        for (int i = 0; i < Q_NBR; ++i) {
            NRF_LOG_INFO("Buff ptr = %p\n", (uint32_t)&mem_pool[i][0]);
            while(NRF_LOG_PROCESS());
        }
    }

    while(NRF_LOG_PROCESS());

    for (int i = 0; i < Q_NBR + 1; ++i) {
        uint8_t* p = NULL;
        bool err = NOTIF_BUFFERS_get_free_buffer(&descr, &p);
        NRF_LOG_INFO("Get %d) (err %d) %p\n", i, err, (uint32_t)p);
        while(NRF_LOG_PROCESS());
    }


    pt_t pt = {0};
    PT_INIT(&pt);

    pt_t pt_test_tim = {0};

    ADC_start_conv();
    // Enter main loop.
    for (;;) {
        app_sched_execute();
//        power_manage();
        while(NRF_LOG_PROCESS());
        BLE_COMM_tx_process();

        (void)PT_SCHEDULE(test_blecomm(&pt));
        (void)PT_SCHEDULE(test_sys_tim(&pt_test_tim));

        APP_LOGIC_pool();
    }
}
